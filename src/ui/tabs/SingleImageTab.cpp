#include "ui/tabs/SingleImageTab.h"
#include "utils/ImageUtils.h"
#include "data/DatasetModel.h"
#include "core/ImageProcessor.h"
#include "core/FeatureExtractor.h"
#include "core/Classifier.h"

#include <QSplitter>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QFileInfo>
#include <QSplitter>
#include <QTimer>
#include <QCheckBox>
#include <QFileSystemModel>

using namespace cv;

SingleImage::SingleImage(MetalSurfaceDetection* parentWindow, QWidget *parent)
{
    setupUI();
    setupConnection();
    setupModelInfo();
}

SingleImage::~SingleImage() {}

void SingleImage::setFileSystemModel(QFileSystemModel* model)
{
    m_fileModel = model;
}

void createResultInfo(const QString& labelText, QLabel*& labelData, QVBoxLayout*& groupLayout)
{
    QLabel* label = new QLabel(labelText);
    labelData = new QLabel();
    QHBoxLayout* layout = new QHBoxLayout();
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(label);
    layout->addWidget(labelData);
    groupLayout->addLayout(layout);
}

void setupImgDisplay(const QString& groupName, QLabel*& imgLabel, QHBoxLayout*& imgGroupLayout)
{
    QGroupBox* groupBox = new QGroupBox(groupName);
    imgLabel = new QLabel();
    QVBoxLayout* layout2 = new QVBoxLayout(groupBox);
    layout2->addWidget(imgLabel);
    imgGroupLayout->addWidget(groupBox);
}

void SingleImage::setupUI()
{
    m_splitter = new QSplitter(Qt::Vertical, this);

    // ===创建上半部分：图片显示区域=== //
    QWidget* imgTabContainer = new QWidget();
    QHBoxLayout* imgGroupLayout = new QHBoxLayout(imgTabContainer);

    setupImgDisplay(tr("原图"), m_originImgLabel, imgGroupLayout);
    setupImgDisplay(tr("预处理图"), m_preprocessImgLabel, imgGroupLayout);
    setupImgDisplay(tr("HOG特征可视化图"), m_HOGImgLabel, imgGroupLayout);

    // ===创建下半部分：信息显示区域（日志、控制、结果）=== //
    QWidget* infoContainer = new QWidget();
    QHBoxLayout* infoLayout = new QHBoxLayout(infoContainer);

    // 日志区域（左）
    QGroupBox* logGroupBox = new QGroupBox("日志信息");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroupBox);
    m_logTextEdit = new QTextEdit();
    logLayout->addWidget(m_logTextEdit);

    QVBoxLayout* ctrlResultLayout = new QVBoxLayout();

    // 控制面板区域（右上）
    QGroupBox* ctrlGroupBox = new QGroupBox("控制区");
    QVBoxLayout* ctrlGroupLayout = new QVBoxLayout(ctrlGroupBox);

    QHBoxLayout* layout11 = new QHBoxLayout();
    QPushButton* saveBtn = new QPushButton("保存");
    QPushButton* changeBtn = new QPushButton("切换模型");
    QPushButton* stopBtn = new QPushButton("停止");
    QPushButton* startBtn = new QPushButton("开始");
    QLabel* autoLabel = new QLabel(" | 自动检测: ");
    autoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_autoCheckBox = new QCheckBox();
    m_autoCheckBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layout11->addWidget(saveBtn);
    layout11->addWidget(changeBtn);
    layout11->addWidget(stopBtn);
    layout11->addWidget(startBtn);
    layout11->addWidget(autoLabel);
    layout11->addWidget(m_autoCheckBox);

    QHBoxLayout* layout22 = new QHBoxLayout();
    QLabel* lightLabel = new QLabel("亮   度：");
    m_brightSlider = new QSlider(Qt::Horizontal);
    m_brightSlider->setFixedWidth(200);
    m_brightSlider->setRange(-100,100);
    m_brightSlider->setSingleStep(0);
    m_brightSlider->setTickInterval(20);
    m_brightValue = new QLabel("0");
    layout22->addWidget(lightLabel);
    layout22->addWidget(m_brightSlider);
    layout22->addWidget(m_brightValue);
    layout22->addStretch();

    QHBoxLayout* layout33 = new QHBoxLayout();
    QLabel* contractLabel = new QLabel("对比度：");
    m_contractSlider = new QSlider(Qt::Horizontal);
    m_contractSlider->setFixedWidth(200);
    m_contractSlider->setRange(-100,100);
    m_contractSlider->setSingleStep(0);
    m_contractSlider->setTickInterval(20);
    m_contractValue = new QLabel("0");
    layout33->addWidget(contractLabel);
    layout33->addWidget(m_contractSlider);
    layout33->addWidget(m_contractValue);
    layout33->addStretch();

    ctrlGroupLayout->addLayout(layout11);
    ctrlGroupLayout->addLayout(layout22);
    ctrlGroupLayout->addLayout(layout33);

    // 结果显示区域（右下）
    QGroupBox* resultGroupBox = new QGroupBox("结果显示");
    QVBoxLayout* resultGroupLayout = new QVBoxLayout(resultGroupBox);

    createResultInfo(tr("模型:"), m_modelStrL, resultGroupLayout);
    createResultInfo(tr("预测结果:"), m_resultL, resultGroupLayout);
    createResultInfo(tr("置信度:"), m_confidenceL, resultGroupLayout);
    createResultInfo(tr("特征维度:"), m_featureDiL, resultGroupLayout);
    createResultInfo(tr("处理耗时:"), m_timeL, resultGroupLayout);

    ctrlResultLayout->addWidget(ctrlGroupBox);
    ctrlResultLayout->addWidget(resultGroupBox);

    infoLayout->addWidget(logGroupBox);
    infoLayout->addLayout(ctrlResultLayout);

    infoLayout->setStretchFactor(logGroupBox, 1);
    infoLayout->setStretchFactor(ctrlResultLayout, 1);

    m_splitter->addWidget(imgTabContainer);
    m_splitter->addWidget(infoContainer);

    // 按钮：保存、切换、停止、开始
    m_saveBtn = saveBtn;
    m_changeBtn = changeBtn;
    m_stopBtn = stopBtn;
    m_startBtn = startBtn;
}

void SingleImage::setupConnection()
{
    connect(m_startBtn, &QPushButton::clicked, this, &SingleImage::onStartBtnClicked);
    connect(m_autoCheckBox, &QCheckBox::checkStateChanged, this, &SingleImage::onAutoCheckBoxChanged);
    connect(m_brightSlider, &QSlider::valueChanged, this, &SingleImage::onBrightSliderChanged);
    connect(m_contractSlider, &QSlider::valueChanged, this, &SingleImage::onContrastSliderChanged);
}

void SingleImage::setupModelInfo()
{
    DatasetModel* instance = DatasetModel::instance();
    QString modelType = instance->getModelType();
    QVariantMap modelParam = instance->getModelParameters();
    QString modelInfo = QString("%1 (C=%2, gamma=%3, kernel=%4)")
                            .arg(modelType)
                            .arg(modelParam["C"].toString())
                            .arg(modelParam["gamma"].toString())
                            .arg(modelParam["kernel"].toString());
    m_modelStrL->setText(modelInfo);
}

void SingleImage::setupSplitter()
{
    m_splitter->resize(this->size());
    int Height = m_splitter->height();
    if(Height>100)
        m_splitter->setSizes({int(Height*0.6), int(Height*0.4)});
    else
        m_splitter->setSizes({600,400});
}

void SingleImage::onTreeViewClicked(const QModelIndex &index)
{
    if(!m_fileModel)
        return;

    QString infoText;

    // 1. 获取选中项目的完整文件路径
    QString filePath = m_fileModel->filePath(index);
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    if(fileInfo.isFile()) // 如果是文件，显示文件详情
    {
        QString dirName = fileInfo.dir().dirName();
        QString sizeStr = QString::number(fileInfo.size() / 1024.0, 'f', 1) + " KB";

        infoText = QString("<b>文件:</b> %1<br>"
                           "<b>类别:</b> %2<br>"
                           "<b>大小:</b> %3<br>"
                           "<b>完整路径:<b><br><code style='word-wrap: break-word;'>%4</code>")
                       .arg(fileName)
                       .arg(dirName)
                       .arg(sizeStr)
                       .arg(filePath.toHtmlEscaped()); // 注意转义HTML特殊字符
        // 工作区显示对应图片
        m_originMat = imread(filePath.toStdString());
        m_oriDisplayMat = m_originMat.clone();
        m_originImgLabel->setPixmap(ImageUtils::matToPixmap(m_originMat, m_originImgLabel));
    }
    else if(fileInfo.isDir()) // 如果是文件夹，计算文件夹下图片数量
    {
        int imgCount = 0;
        QDir dir(filePath);
        QStringList imgFiles = dir.entryList(QStringList() << "*.bmp", QDir::Files);
        imgCount = imgFiles.size();
        infoText = QString("<b>文件夹:</b> %1<br>"
                           "<b>图片数量:</b> %2<br>"
                           "<b>完整路径:<b><br><code style='word-wrap: break-word;'>%3</code>")
                       .arg(fileName)
                       .arg(imgCount)
                       .arg(filePath.toHtmlEscaped()); // 注意转义HTML特殊字符
        // 清空预测结果
        resetResultInfo();
    }

    emit sendFileInfo(infoText);

    if(m_isAuto)
        onStartBtnClicked();
}

void SingleImage::onStartBtnClicked()
{
    if(m_originMat.empty())
    {
        qWarning() << "请先选择图片！";
        return;
    }

    // 1. 记录总开始时间
    QElapsedTimer timer;
    timer.start();

    // 2. 预处理
    cv::Mat preprocessMat;
    qint64 precessingTime = 0;
    preprocessMat = ImageProcessor::preprocess(m_originMat, precessingTime);

    // 3. 特征提取
    ExtractHOGStats stats;
    std::vector<float> features = FeatureExtractor::extractHOG(preprocessMat, stats);

    // 4. 分类
    qint64 classifyTime;
    ClassifyStats result;
    Classifier::classify(features, result);

    // 4. 计算总耗时
    qint64 totalTime = timer.elapsed();

    // 5. 更新UI
    updateDetectionResult(totalTime, precessingTime, stats, result);
    displayImages(preprocessMat, stats.visualizationl);
}

void SingleImage::updateDetectionResult(qint64 totalTime, qint64 preprocessTime,
                                        const ExtractHOGStats& hog, const ClassifyStats& result)
{
    // 更新日志
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    m_logTextEdit->append(QString("%1 - 已加载图片").arg(timestamp));
    m_logTextEdit->append(QString("%1 - 预处理耗时: %2ms").arg(timestamp).arg(preprocessTime));
    m_logTextEdit->append(QString("%1 - 特征提取耗时: %2ms, 特征维度: %3").arg(timestamp).arg(hog.extractionTime).arg(hog.featureDimension));
    m_logTextEdit->append(QString("%1 - 分类耗时: %2ms").arg(timestamp).arg(result.classifyTime));
    m_logTextEdit->append(QString("%1 - 预测完成: %2 (置信度: %3)")
                              .arg(timestamp)
                              .arg(result.label)
                              .arg(QString("%1%").arg(result.confidence * 100)));

    // 更新结果显示区
    m_resultL->setText(result.label);
    m_confidenceL->setText(QString("%1%").arg(result.confidence * 100));
    m_featureDiL->setText(QString("%1").arg(hog.featureDimension));
    m_timeL->setText(QString("%1ms").arg(totalTime));
}

void SingleImage::displayImages(const cv::Mat& preprocessedMat, const cv::Mat& hogVisualization)
{
    m_preprocessImgLabel->setPixmap(ImageUtils::matToPixmap(preprocessedMat, m_preprocessImgLabel));
    m_HOGImgLabel->setPixmap(ImageUtils::matToPixmap(hogVisualization, m_HOGImgLabel));
}

void SingleImage::resetResultInfo()
{
    m_resultL->clear();
    m_confidenceL->clear();
    m_featureDiL->clear();
    m_timeL->clear();
    m_preprocessImgLabel->clear();
    m_HOGImgLabel->clear();
}

void SingleImage::onAutoCheckBoxChanged()
{
    m_isAuto = m_autoCheckBox->isChecked();
}

void SingleImage::onBrightSliderChanged(int value)
{
    m_brightValue->setText(QString("%1").arg(value));

    int rows = m_oriDisplayMat.rows;
    int cols = m_oriDisplayMat.cols;
    int channels = m_oriDisplayMat.channels();

    for(int i = 0; i < rows; ++i)
    {
        // 获取第i行指针
        uchar* rowPtr = m_oriDisplayMat.ptr<uchar>(i);
        for(int j = 0; j < cols; ++j)
        {
            // 对每个通道进行调整
            for(int c = 0; c < channels; ++c)
            {
                int pixelValue = rowPtr[j * channels + c] + value;
                rowPtr[j * channels + c] = qBound(0, pixelValue, 255);
            }
        }
    }
    m_originImgLabel->setPixmap(ImageUtils::matToPixmap(m_oriDisplayMat, m_originImgLabel));
}

void SingleImage::onContrastSliderChanged(int value)
{
    m_contractValue->setText(QString("%1").arg(value));

    double factor = 1.0 + value / 100.0;
    int rows = m_oriDisplayMat.rows;
    int cols = m_oriDisplayMat.cols;
    int channels = m_oriDisplayMat.channels();

    // 线性对比度调整
    for(int i = 0; i < rows; ++i)
    {
        // 获取第i行指针
        uchar* rowPtr = m_oriDisplayMat.ptr<uchar>(i);
        for(int j = 0; j < cols * channels; ++j)
        {
            double newValue = factor * (rowPtr[j] - 128) + 128;
            rowPtr[j] = qBound(0.0, newValue, 255.0);
        }
    }
    m_originImgLabel->setPixmap(ImageUtils::matToPixmap(m_oriDisplayMat, m_originImgLabel));
}

