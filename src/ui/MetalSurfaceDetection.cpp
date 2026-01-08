#include "ui/MetalSurfaceDetection.h"
#include "ui_MetalSurfaceDetection.h"
#include "ui/tabs/SingleImageTab.h"
#include "ui/tabs/BatchValidationTab.h"

#include <QPixmap>
#include <QDir>
#include <QFile>
#include <QTreeView>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>
#include <QTimer>
#include <QSplitter>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileSystemModel>

#include <opencv2/opencv.hpp>

using namespace cv;

void load();
void usemodel();


MetalSurfaceDetection::MetalSurfaceDetection(QWidget *parent)
    : QWidget(parent)
    , m_singleImageTab(nullptr)
    , m_batchValidationTab(nullptr)
    , ui(new Ui::MetalSurfaceDetection)
{
    ui->setupUi(this);
    this->setWindowTitle("金属表面检测系统");

    setupUI();
}

void MetalSurfaceDetection::setupUI()
{
    // 创建界面主分割器
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->resize(this->size());

    m_singleImageTab = new SingleImage(this);
    m_batchValidationTab = new BatchValidation(this);

    QWidget* leftPanel = createLeftPanel();
    QWidget* tabsPanel = createTabsPanel();

    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(tabsPanel);

    connect(this, &MetalSurfaceDetection::setupTabsSplitter, m_singleImageTab, &SingleImage::setupSplitter);

    // 使用定时器延时设置控件大小
    QTimer::singleShot(0, this, [this](){
        m_mainSplitter->setSizes({200,800});

        int leftHeight = m_leftSplitter->height();
        if(leftHeight>100)
            m_leftSplitter->setSizes({int(leftHeight*0.05), int(leftHeight*0.75), int(leftHeight*0.2)});
        else
            m_leftSplitter->setSizes({50, 600,200});
        emit setupTabsSplitter();
    });
}

MetalSurfaceDetection::~MetalSurfaceDetection() { delete ui; }

QWidget* MetalSurfaceDetection::createLeftPanel()
{
    // 创建垂直分割器作为左侧区域的容器
    QSplitter* splitter = new QSplitter(Qt::Vertical);

    // 创建上部分：容器-->放置“更改目录”按钮
    QWidget* buttonContainer = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(2,2,2,2);
    m_changeDirBtn = new QPushButton("更改目录");
    buttonLayout->addWidget(m_changeDirBtn);

    // 创建中部分：文件导航
    m_fileTreeView = new QTreeView();
    m_fileModel = new QFileSystemModel(this);
    QString dataRootPath = "D:/Qt_src/opencvTest/TestTest";
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_fileModel->setNameFilters(QStringList() << "*.bmp");
    m_fileModel->setNameFilterDisables(false); // 隐藏不匹配的文件
    QModelIndex index = m_fileModel->setRootPath(dataRootPath);
    m_fileTreeView->setModel(m_fileModel);
    m_fileTreeView->setRootIndex(index);

    // 配置视图外观和行为
    m_fileTreeView->setHeaderHidden(false); // 显示表头（名称、大小、类型等）
    m_fileTreeView->setColumnWidth(0, 200); // 设置“名称”列的初始宽度
    m_fileTreeView->hideColumn(1); // 隐藏“大小”列（根据需要）
    m_fileTreeView->hideColumn(2); // 隐藏“类型”列
    m_fileTreeView->hideColumn(3); // 隐藏“修改日期”列
    m_fileTreeView->setSortingEnabled(true); // 启用排序（点击表头可排序）
    m_fileTreeView->setMaximumWidth(400);
    m_fileTreeView->setMinimumWidth(150);

    // 创建下半部分：统计信息面板（使用QGroupBox获得带标题的边框）
    QGroupBox* statsGroupBox = new QGroupBox();
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroupBox);
    m_infoLabel = new QLabel("请从文件树中选择：");
    m_infoLabel->setWordWrap(true); // 启用自动换行
    statsLayout->addWidget(m_infoLabel);

    splitter->addWidget(buttonContainer);
    splitter->addWidget(m_fileTreeView);
    splitter->addWidget(statsGroupBox);

    connect(m_changeDirBtn, &QPushButton::clicked, this, &MetalSurfaceDetection::onChangeDirBtnClicked);
    connect(m_fileTreeView, &QTreeView::clicked, m_singleImageTab, &SingleImage::resetResultInfo);
    connect(m_fileTreeView, &QTreeView::clicked, m_singleImageTab, &SingleImage::onTreeViewClicked);
    connect(m_singleImageTab, &SingleImage::sendFileInfo, this, &MetalSurfaceDetection::receiveFileInfo);

    m_leftSplitter = splitter;

    return splitter;
}

QString MetalSurfaceDetection::rootPath() const
{
    return m_fileModel->rootPath();
}

QWidget* MetalSurfaceDetection::createTabsPanel()
{
    // 创建上半部分：TabWidget里有4个标签页
    QTabWidget* tabWidget = new QTabWidget();

    tabWidget->addTab(m_singleImageTab, "单张图片测试");
    tabWidget->addTab(m_batchValidationTab, "批量验证和测试");

    m_singleImageTab->setFileSystemModel(m_fileModel);
    m_batchValidationTab->setRootPath(m_fileModel->rootPath());
    return tabWidget;
}

void MetalSurfaceDetection::receiveFileInfo(const QString& fileInfo)
{
    m_infoLabel->setText(fileInfo);
}

void MetalSurfaceDetection::onChangeDirBtnClicked()
{
    QString newDir = QFileDialog::getExistingDirectory(this, "选择数据目录", m_fileModel->rootPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(newDir.isEmpty() || !QDir(newDir).exists())
        return;
    QDir dir(newDir);
    if(!dir.exists("train") && !dir.exists("valid") && !dir.exists("test"))
        QMessageBox::warning(this, "目录结构警告", "所选目录中未找到 train, valid, test 等标准文件夹。\n"
                                                   "请确保选择正确的数据根目录。");
    // 更新模型和视图
    QModelIndex index = m_fileModel->setRootPath(newDir);
    m_fileTreeView->setModel(m_fileModel);
    m_fileTreeView->setRootIndex(index);

    m_batchValidationTab->setRootPath(newDir);
}
