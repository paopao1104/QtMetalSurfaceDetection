#include "ui/tabs/BatchValidationTab.h"
#include "data/DatasetModel.h"
#include "ui/tabs/PrecisionRecallBarChart.h"
#include "utils/Logger.h"
#include "core/ImageProcessor.h"
#include "core/FeatureExtractor.h"
#include "core/Classifier.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QRadioButton>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QFile>
#include <QProgressBar>
#include <QVector>
#include <QThread>
#include <QMetaObject>
#include <QDirIterator>

BatchValidationWorker::BatchValidationWorker(QObject* parent) : QObject(parent), m_stopRequested(false)
{
}

void BatchValidationWorker::startBatchValidation(const QString& datasetPath, const QString& modelPath, const QString& datasetType)
{
    m_stopRequested = false;

    try
    {
        // 1. 加载模型
        LOG_INFO(QString("开始加载模型: %1").arg(modelPath));
        if(!DatasetModel::instance()->loadModel(modelPath))
        {
            emit errorOccurred(QString("模型加载失败: %1").arg(modelPath));
            return;
        }

        // 2. 准备图像处理器和特征提取器
        ImageProcessor imgProcessor;
        FeatureExtractor featureExtractor;
        Classifier classifier;

        // 3. 获取数据集文件列表
        QStringList imgExtensions = {"*.bmp"};
        QVector<QString> imgFiles;
        QDirIterator it(datasetPath, imgExtensions, QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            imgFiles.append(it.next());
        }

        if(imgFiles.empty())
        {
            emit errorOccurred(QString("数据集为空: %1").arg(datasetPath));
            return;
        }

        LOG_INFO(QString("开始批量验证, 共 %1 个图像").arg(imgFiles.size()));

        // 4. 处理每个图像
        QVector<PredictionResult> allResults;

        for(int i = 0; i < imgFiles.size(); i++)
        {
            if(m_stopRequested)
            {
                LOG_INFO("收到停止请求, 中断验证");
                emit validationStopped();
                return;
            }

            const QString& imgPath = imgFiles[i];

            // 发送进度更新
            emit progressUpdated(i+1, imgFiles.size(), imgPath);

            // 处理当前图像
            QElapsedTimer timer;
            timer.start();

            PredictionResult result;
            result.imgName = QFileInfo(imgPath).fileName();
            result.imgPath = imgPath;
            result.timeStamp = QDateTime::currentDateTime();

            try
            {
                cv::Mat img = cv::imread(imgPath.toStdString());
                if(img.empty())
                {
                    result.processingSuccess = false;
                    result.errorMessage = "图像加载失败";
                    allResults.append(result);
                    continue;
                }

                result.imgSize = cv::Size(img.cols, img.rows);

                QFileInfo fileInfo(imgPath);
                QDir imgDir = fileInfo.dir();
                QString dirName = imgDir.dirName();
                result.trueLabel = dirName;

                // 预处理
                qint64 preprocessTime;
                cv::Mat preprocessedImg = imgProcessor.preprocess(img, preprocessTime);


                // 提取特征
                ExtractHOGStats hogStats;
                std::vector<float> features;
                features = featureExtractor.extractHOG(preprocessedImg, hogStats);

                // 分类
                ClassifyStats classifyStats;
                classifier.classify(features, classifyStats);

                result.predictClassId = classifyStats.classId;
                result.predictLabel = classifyStats.label;

                // 填充剩余信息
                result.processingSuccess = true;
                result.totalProcessingTime = timer.elapsed();
                result.modelName = DatasetModel::instance()->getModelType();
                result.modelPath = DatasetModel::instance()->getModelPath();
            }
            catch(const cv::Exception& e)
            {
                result.processingSuccess = false;
                result.errorMessage = QString("OpenCV 异常: %1").arg(e.what());
            }
            catch(const std::exception& e)
            {
                result.processingSuccess = false;
                result.errorMessage = QString("标准异常: %1").arg(e.what());
            }

            // 添加结果
            allResults.append(result);

            // 发送单个结果
            emit singleResultReady(result);
        }

        // 发送完成信号
        emit batchCompleted(allResults);

        LOG_INFO(QString("批量验证完成: 处理 %1 个图像").arg(allResults.size()));
    }
    catch(const std::exception& e){
        emit errorOccurred(QString("验证过程异常：%1").arg(e.what()));
    }
}

void BatchValidationWorker::stopBatchValidation()
{
    m_stopRequested = true;
}

// =================== BatchValidation ===================== //

BatchValidation::BatchValidation(MetalSurfaceDetection* parentWindow, QWidget *parent)
{
    setupUI();

    // 创建工作线程
    m_workerThread = new QThread(this);
    m_worker = new BatchValidationWorker();
    m_worker->moveToThread(m_workerThread);

    m_workerThread->start();

    setupConnection();
}

BatchValidation::~BatchValidation() {}

void BatchValidation::setRootPath(const QString& rootPath)
{
    m_rootPath = rootPath;
    QDir dir(rootPath);
    m_hasDataset = dir.exists("valid") && dir.exists("test");
}

void BatchValidation::setupConnection()
{
    connect(m_testRadioBtn, &QRadioButton::clicked, this, &BatchValidation::onRadioBtnClicked);
    connect(m_testRadioBtn, &QRadioButton::clicked, this, &BatchValidation::onRadioBtnClicked);
    connect(m_modelCombo, &QComboBox::currentIndexChanged, this, &BatchValidation::onModelComboChanged);
    connect(m_startBtn, &QPushButton::clicked, this, &BatchValidation::onStartBatchValidation);
    connect(m_stopBtn, &QPushButton::clicked, this, &BatchValidation::onStopBatchValidation);
    connect(m_csvBtn, &QPushButton::clicked, this, &BatchValidation::onExportCSVReportBtnClicked);
    connect(m_visualizationBtn, &QPushButton::clicked, this, &BatchValidation::onGenerateVisualrReportBtnClicked);
    connect(m_errorBtn, &QPushButton::clicked, this, &BatchValidation::onViewErrorSampleBtnClicked);

    // 工作线程
    connect(m_worker, &BatchValidationWorker::progressUpdated, this, &BatchValidation::handleProgressUpdate);
    connect(m_worker, &BatchValidationWorker::singleResultReady, this, &BatchValidation::handleSingleResult);
    connect(m_worker, &BatchValidationWorker::validationStopped, this, &BatchValidation::handleValidationStopped);
}

void BatchValidation::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // 左上：控制区
    QVBoxLayout* leftLayout = new QVBoxLayout();
    QWidget* ctrlContainer = new QWidget();
    QVBoxLayout* ctrlContainerLayout = new QVBoxLayout(ctrlContainer);

    // 数据集选择
    QHBoxLayout* datasetLayout = new QHBoxLayout();
    QLabel* label1 = new QLabel("数据集选择 : ");
    label1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_validRadioBtn = new QRadioButton("valid");
    m_testRadioBtn = new QRadioButton("test");
    datasetLayout->addWidget(label1);
    datasetLayout->addWidget(m_validRadioBtn);
    datasetLayout->addWidget(m_testRadioBtn);
    datasetLayout->addStretch();

    // 选择模型
    QHBoxLayout* modelLayout = new QHBoxLayout();
    QLabel* label2 = new QLabel("模型 : ");
    label2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_modelCombo = new QComboBox();
    m_modelCombo->setFixedWidth(150);

    m_modelsPath = "../../models/";
    QDir modelDir(m_modelsPath);
    QStringList modelFiles = modelDir.entryList(QStringList() << "*.yml" << "*.xml", QDir::Files);
    foreach (QString fileName, modelFiles) {
        m_modelCombo->addItem(fileName);
    }

    modelLayout->addWidget(label2);
    modelLayout->addWidget(m_modelCombo);
    modelLayout->addStretch();

    // 按钮：开始、停止
    QHBoxLayout* startStopLayout = new QHBoxLayout();
    m_startBtn = new QPushButton("开始验证");
    m_startBtn->setFixedWidth(80);
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setFixedWidth(80);
    startStopLayout->addWidget(m_startBtn);
    startStopLayout->addWidget(m_stopBtn);
    startStopLayout->addStretch();

    // 进度显示
    QHBoxLayout* progressLayout = new QHBoxLayout();
    QLabel* label3 = new QLabel("进度 : ");
    label3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    progressLayout->addWidget(label3);
    progressLayout->addWidget(m_progressBar);
    progressLayout->addStretch();

    // 状态
    QHBoxLayout* statusLayout = new QHBoxLayout();
    QLabel* label4 = new QLabel("状态 : ");
    label3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_statusLabel = new QLabel("模型未加载");
    statusLayout->addWidget(label4);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();

    // 按钮：导出csv报告、生成可视化报告、查看错误样本
    QHBoxLayout* reportsLayout = new QHBoxLayout();
    m_csvBtn = new QPushButton("导出csv报告");
    m_csvBtn->setFixedWidth(120);
    m_visualizationBtn = new QPushButton("生成可视化报告");
    m_visualizationBtn->setFixedWidth(120);
    m_errorBtn = new QPushButton("查看错误样本");
    m_errorBtn->setFixedWidth(120);
    reportsLayout->addWidget(m_csvBtn);
    reportsLayout->addStretch(40);
    reportsLayout->addWidget(m_visualizationBtn);
    reportsLayout->addStretch(40);
    reportsLayout->addWidget(m_errorBtn);
    reportsLayout->addStretch();

    ctrlContainerLayout->addLayout(datasetLayout);
    ctrlContainerLayout->addLayout(modelLayout);
    ctrlContainerLayout->addLayout(startStopLayout);
    ctrlContainerLayout->addLayout(progressLayout);
    ctrlContainerLayout->addLayout(statusLayout);
    ctrlContainerLayout->addLayout(reportsLayout);

    // 左下：详细数据与报告区
    QGroupBox* resultGroup = new QGroupBox("详细数据与报告区");
    QVBoxLayout* resultGroupLayout = new QVBoxLayout(resultGroup);
    setupTable();
    resultGroupLayout->addWidget(m_resultsTabel);

    leftLayout->addWidget(ctrlContainer, 1);
    leftLayout->addWidget(resultGroup, 3);

    // ============== 右边：混淆矩阵热力图+各类别精确率-召回率条形图
    QVBoxLayout* rightLayout = new QVBoxLayout();

    // 右上：混淆矩阵热力图
    QGroupBox* confusionGroup = new QGroupBox("混淆矩阵热力图");
    QVBoxLayout* confusionGroupLayout = new QVBoxLayout(confusionGroup);
    QTableWidget* confusionTable = new QTableWidget();
    confusionGroupLayout->addWidget(confusionTable);

    // 右下：各类别精确率-召回率条形图
    PrecisionRecallBarChart* prChart = new PrecisionRecallBarChart();
    QVector<ClassMetric> metrics;
    metrics.push_back(ClassMetric{"裂纹", 80, 98, 0.2, 2});
    metrics.push_back(ClassMetric{"斑块", 10, 97, 0.3, 4});
    metrics.push_back(ClassMetric{"夹杂物", 20, 90, 0.4, 5});
    metrics.push_back(ClassMetric{"腐蚀", 20, 90, 0.4, 5});
    metrics.push_back(ClassMetric{"轧痕", 20, 90, 0.4, 5});
    metrics.push_back(ClassMetric{"划痕", 20, 90, 0.4, 5});
    prChart->setData(metrics);

    rightLayout->addWidget(confusionGroup, 1);
    rightLayout->addWidget(prChart, 1);

    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);
}

void BatchValidation::setupComponents()
{

}

void BatchValidation::setupCharts()
{

}

void BatchValidation::setupTable()
{
    m_resultsTabel = new QTableWidget();
    // 设置表格列
    QStringList headers = {"文件名", "真实类别", "预测类别", "置信度", "是否正确", "错误类型", "处理耗时"};
    m_resultsTabel->setColumnCount(headers.size());
    m_resultsTabel->setHorizontalHeaderLabels(headers);

    // 设置列宽
    int width = m_resultsTabel->width();
    m_resultsTabel->setColumnWidth(0, width / 7);
    m_resultsTabel->setColumnWidth(1, width / 7);
    m_resultsTabel->setColumnWidth(2, width / 7);
    m_resultsTabel->setColumnWidth(3, width / 7);
    m_resultsTabel->setColumnWidth(4, width / 7);
    m_resultsTabel->setColumnWidth(5, width / 7);
    m_resultsTabel->setColumnWidth(6, width / 7 - 20);

    // 设置排序
    // m_resultTable->setSortingEnabled(true);
}

void BatchValidation::onRadioBtnClicked()
{
    if(!m_hasDataset)
    {
        QMessageBox::warning(this, "目录结构警告", "所选目录中未找到 train, valid等标准文件夹。\n"
                                                   "请确保选择正确的数据根目录。");
        return;
    }

    if(!m_validRadioBtn->isChecked() && !m_validRadioBtn->isChecked())
        return;

    bool isValid = m_validRadioBtn->isChecked();
    m_datasetPath = m_rootPath + (isValid ? "/valid" : "/test");
}

void BatchValidation::onModelComboChanged(int index)
{
}

void BatchValidation::onStartBatchValidation()
{
    if(m_isValidating)
    {
        QMessageBox::warning(this, "警告", "验证正在进行中。");
        return;
    }

    if(!m_hasDataset)
    {
        QMessageBox::warning(this, "目录结构警告", "所选目录中未找到 train, valid等标准文件夹。\n" "请确保选择正确的数据根目录。");
        return;
    }

    QString curModelPath = m_modelsPath + m_modelCombo->currentText();
    QFile file(curModelPath);
    if(!file.exists())
    {
        QMessageBox::warning(this, "警告", "当前模型文件不存在。\n" "请确保选择正确的模型文件。");
        return;
    }

    // 清空之前的结果
    clearResults(); // 待实现

    // 更新UI状态
    updateValidationControls(true); // 待实现
    m_statusLabel->setText("正在准备验证...");

    // 记录开始时间
    m_validationTime = QDateTime::currentDateTime();

    LOG_INFO(QString("开始批量验证 = 数据集: %1, 模型: %2").arg(m_datasetPath).arg(curModelPath));

    // 启动验证工作线程
    QMetaObject::invokeMethod(m_worker, "startBatchValidation",
                                        Q_ARG(QString, m_datasetPath),
                                        Q_ARG(QString, curModelPath),
                                        Q_ARG(QString, QString()));
}

void BatchValidation::onStopBatchValidation()
{
    if(!m_isValidating)
        return;

    // 请求停止验证
    QMetaObject::invokeMethod(m_worker, "stopBatchValidation");

    m_statusLabel->setText("正在停止验证...");
    m_stopBtn->setEnabled(false);

    LOG_INFO("用户请求停止批量验证");
}

void BatchValidation::handleProgressUpdate(int current, int total, const QString& curretFile)
{
    if(total <= 0)
        return;
    int progress = static_cast<int>((static_cast<double>(current) / total) * 100);
    m_progressBar->setValue(progress);

    QFileInfo file(curretFile);
    m_statusLabel->setText(QString("正在验证：%1/%2 (%3/%4)")
                               .arg(file.dir().dirName()).arg(file.fileName())
                               .arg(current).arg(total));
}

void BatchValidation::handleSingleResult(const PredictionResult& result)
{
    // 添加到结果列表
    m_currentResults.append(result);

    // 更新表格
    addResultToTable(result);

    // 实时更新统计信息（每处理10个样本更新一次）
    if(m_currentResults.size() % 10 == 0)
    {

    }
}

void BatchValidation::addResultToTable(const PredictionResult& result)
{
    int row = m_resultsTabel->rowCount();
    m_resultsTabel->insertRow(row);

    // 文件名
    QTableWidgetItem* fileNameItem = new QTableWidgetItem(result.imgName);

    m_resultsTabel->setItem(row, 0, fileNameItem);

    // 真实类别
    QTableWidgetItem* trueLabelItem = new QTableWidgetItem(result.trueLabel);
    m_resultsTabel->setItem(row, 1, trueLabelItem);

    // 预测类别
    QTableWidgetItem* predictLabelItem = new QTableWidgetItem(result.predictLabel);
    m_resultsTabel->setItem(row, 2, predictLabelItem);

    // 置信度
    QTableWidgetItem* confidenceItem = new QTableWidgetItem(result.confidence);
    m_resultsTabel->setItem(row, 3, confidenceItem);

    // 是否正确
    bool correct = result.trueLabel == result.predictLabel;
    QString cStr = correct ? "✅" : "❌";
    QTableWidgetItem* correctItem = new QTableWidgetItem(cStr);
    m_resultsTabel->setItem(row, 4, correctItem);

    // 错误类型
    QTableWidgetItem* errorTypeItem = new QTableWidgetItem("error");
    m_resultsTabel->setItem(row, 5, errorTypeItem);

    // 处理耗时
    QTableWidgetItem* timeItem = new QTableWidgetItem(result.totalProcessingTime);
    m_resultsTabel->setItem(row, 6, timeItem);
}

void BatchValidation::handleBatchCompleted(const QVector<PredictionResult>& results)
{

}

void BatchValidation::handleValidationStopped()
{

}

void BatchValidation::handleErrorOccurred(const QString& error)
{

}


void BatchValidation::onExportCSVReportBtnClicked()
{

}

void BatchValidation::onGenerateVisualrReportBtnClicked()
{

}

void BatchValidation::onViewErrorSampleBtnClicked()
{

}

void BatchValidation::clearResults()
{

}

void BatchValidation::updateValidationControls(bool running)
{

}
