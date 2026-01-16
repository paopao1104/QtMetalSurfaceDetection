#include "ui/tabs/BatchValidationTab.h"
#include "ui/tabs/PrecisionRecallBarChart.h"
#include "ui/tabs/ConfusionMatrixWidget.h"
#include "utils/Logger.h"
#include "data/DatasetModel.h"
#include "core/ImageProcessor.h"
#include "core/FeatureExtractor.h"
#include "core/Classifier.h"
#include "core/TaskQueue.h"
#include "core/Consumer.h"

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
#include <QFileDialog>
#include <QStandardPaths>

BatchValidationWorker::BatchValidationWorker(QObject* parent)
    : QObject(parent),
    m_stopRequested(false),
    curSize(1),
    totalSize(0)
{
}

void BatchValidationWorker::startBatchValidation(const QString& datasetPath, const QString& modelPath, const QString& datasetType)
{
    // singleThread(datasetPath, modelPath, datasetType);
    multiThreads(datasetPath, modelPath, datasetType);
}

void BatchValidationWorker::stopBatchValidation()
{
    m_stopRequested = true;
}

void BatchValidationWorker::onResultReady(const PredictionResult& result, bool imgEmpty)
{
    m_allResults.append(result);

    if(!imgEmpty)
        emit singleResultReady(result);
}

void BatchValidationWorker::onConsumerUpdated(const QString& imgPath)
{
    emit progressUpdated(curSize++, totalSize, imgPath);
}

void BatchValidationWorker::singleThread(const QString& datasetPath, const QString& modelPath, const QString& datasetType)
{
    m_stopRequested = false;
    curSize = 1;
    totalSize = 0;

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
                result.trueClassId = result.getClassIdFromName(dirName);

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

void BatchValidationWorker::multiThreads(const QString& datasetPath, const QString& modelPath, const QString& datasetType)
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
            imgFiles.append(it.next());

        if(imgFiles.empty())
        {
            emit errorOccurred(QString("数据集为空: %1").arg(datasetPath));
            return;
        }

        totalSize = imgFiles.size();
        LOG_INFO(QString("开始批量验证, 共 %1 个图像").arg(imgFiles.size()));

        // 创建任务队列
        m_taskQueue = new TaskQueue();
        for(const QString& imgPath : imgFiles)
            m_taskQueue->enqueue(imgPath);
        m_taskQueue->setFinished(); // 生产完成

        // 创建消费者线程
        int threadCount = QThread::idealThreadCount();
        QList<Consumer*> consumers;
        for(int i = 0 ; i < threadCount; ++i)
        {
            Consumer* consumer = new Consumer(m_taskQueue);
            connect(consumer, &Consumer::resultReady, this, &BatchValidationWorker::onResultReady);
            connect(consumer, &Consumer::progressUpdated, this, &BatchValidationWorker::onConsumerUpdated);
            connect(consumer, &QThread::finished, consumer, &Consumer::deleteLater);
            consumers.append(consumer);
        }

        // 连接所有消费者线程完成信号，我们需要知道何时所有线程完成
        // 使用计数器，每个线程完成时，计数器减1
        m_runningThreads = consumers.size();
        for(Consumer* consumer : consumers)
        {
            connect(consumer, &QThread::finished, this, [this](){
                m_runningThreads--;
                if(m_runningThreads == 0)
                {
                    emit batchCompleted(m_allResults);
                    LOG_INFO(QString("批量验证完成: 处理 %1 个图像").arg(m_allResults.size()));
                }
            });

            consumer->start();
        }
    }
    catch(const std::exception& e){
        emit errorOccurred(QString("验证过程异常：%1").arg(e.what()));
    }
}

// =================== BatchValidation ===================== //

BatchValidation::BatchValidation(MetalSurfaceDetection* parentWindow, QWidget *parent)
{
    setupUI();

    setupWokerThread(); // 创建工作线程

    setupConnection();
}

BatchValidation::~BatchValidation()
{
    if(m_workerThread && m_workerThread->isRunning())
    {
        m_workerThread->quit();
        // m_workerThread->wait(); // 可能会阻塞UI

        // 异步处理(不阻塞UI)
        connect(m_workerThread, &QThread::finished, this, [this](){
            m_workerThread->deleteLater();
            m_workerThread = nullptr;
            qDebug() << "Thread stopped and cleaned up";
        });
    }
}

void BatchValidation::setRootPath(const QString& rootPath)
{
    m_rootPath = rootPath;
    QDir dir(rootPath);
    m_hasDataset = dir.exists("valid") && dir.exists("test");
    datasetPathChanged();
}

void BatchValidation::setupWokerThread()
{
    m_workerThread = new QThread(this);
    m_worker = new BatchValidationWorker();
    m_worker->moveToThread(m_workerThread);

    // 连接线程结束信号
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    m_workerThread->start();
}

void BatchValidation::setupConnection()
{
    connect(m_validRadioBtn, &QRadioButton::clicked, this, &BatchValidation::datasetPathChanged);
    connect(m_testRadioBtn, &QRadioButton::clicked, this, &BatchValidation::datasetPathChanged);
    connect(m_startBtn, &QPushButton::clicked, this, &BatchValidation::onStartBatchValidation);
    connect(m_stopBtn, &QPushButton::clicked, this, &BatchValidation::onStopBatchValidation);
    connect(m_csvBtn, &QPushButton::clicked, this, &BatchValidation::onExportCSVReportBtnClicked);
    connect(m_visualizationBtn, &QPushButton::clicked, this, &BatchValidation::onGenerateVisualrReportBtnClicked);
    connect(m_errorBtn, &QPushButton::clicked, this, &BatchValidation::onViewErrorSampleBtnClicked);

    // 工作线程
    connect(m_worker, &BatchValidationWorker::progressUpdated, this, &BatchValidation::handleProgressUpdate);
    connect(m_worker, &BatchValidationWorker::singleResultReady, this, &BatchValidation::handleSingleResult);
    connect(m_worker, &BatchValidationWorker::validationStopped, this, &BatchValidation::onStopBatchValidation);
    connect(m_worker, &BatchValidationWorker::batchCompleted, this, &BatchValidation::handleBatchCompleted);
    connect(m_worker, &BatchValidationWorker::errorOccurred, this, &BatchValidation::handleErrorOccurred);
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
    m_confusionMatrixPlot = new ConfusionMatrixWidget();

    // 右下：各类别精确率-召回率条形图
    m_precisionRecallChart = new PrecisionRecallBarChart();

    rightLayout->addWidget(m_confusionMatrixPlot, 1);
    rightLayout->addWidget(m_precisionRecallChart, 1);

    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 2);
}

void BatchValidation::setupTable()
{
    m_resultsTabel = new QTableWidget();
    // 设置表格列
    QStringList headers = {"文件名", "真实类别", "预测类别", "是否正确"};
    m_resultsTabel->setColumnCount(headers.size());
    m_resultsTabel->setHorizontalHeaderLabels(headers);

    // 设置列宽
    // int width = m_resultsTabel->width();
    // m_resultsTabel->setColumnWidth(0, width / 7);
    // m_resultsTabel->setColumnWidth(1, width / 7);
    // m_resultsTabel->setColumnWidth(2, width / 7);
    // m_resultsTabel->setColumnWidth(3, width / 7);

    // 设置排序
    m_resultsTabel->setSortingEnabled(true);
}

void BatchValidation::datasetPathChanged()
{
    if(!m_hasDataset)
    {
        QMessageBox::warning(this, "目录结构警告", "所选目录中未找到 train, valid等标准文件夹。\n"
                                                   "请确保选择正确的数据根目录。");
        return;
    }

    if(!m_validRadioBtn->isChecked() && !m_testRadioBtn->isChecked())
        return;

    bool isValid = m_validRadioBtn->isChecked();

    m_datasetPath = m_rootPath + (isValid ? "/valid" : "/test");
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
    clearResults();

    // 更新UI状态
    updateValidationControls(true);
    m_statusLabel->setText("正在准备验证...");

    // 记录开始时间
    m_validationStartTime = QDateTime::currentDateTime();

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
        updateConfusionMatrixPlot();
        updatePrecisionRecallChart();
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

    // 是否正确
    bool correct = result.trueLabel == result.predictLabel;
    QString cStr = correct ? "✅" : "❌";
    QTableWidgetItem* correctItem = new QTableWidgetItem(cStr);
    m_resultsTabel->setItem(row, 3, correctItem);
}

void BatchValidation::updateConfusionMatrixPlot()
{
    m_confusionMatrixPlot->setData(m_currentResults);
}

void BatchValidation::updatePrecisionRecallChart()
{
    m_precisionRecallChart->setData(m_currentResults);
}

void BatchValidation::handleBatchCompleted(const QVector<PredictionResult>& results)
{
    m_isValidating = false;
    m_hasResults = true;

    // 计算总耗时
    QDateTime endTime = QDateTime::currentDateTime();
    qint64 totalSeconds = m_validationStartTime.secsTo(endTime);

    // 最终计算和更新；
    updateConfusionMatrixPlot();
    updatePrecisionRecallChart();

    // 更新UI状态
    updateValidationControls(false);
    m_progressBar->setValue(100);
    m_statusLabel->setText(QString("验证完成！共处理 %1 个样本，耗时 %2 秒").arg(results.size()).arg(totalSeconds));

    // 启用导出按钮
    m_csvBtn->setEnabled(true);
    m_visualizationBtn->setEnabled(true);
    m_errorBtn->setEnabled(m_currentResults.size() > 0);
}

void BatchValidation::handleErrorOccurred(const QString& error)
{

}

bool BatchValidation::exportToCSV(const QString& filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 写入表头
    out << "文件名,真实类别,预测类别,置信度,是否正确,错误类型,处理耗时(ms),图像路径,时间戳\n";

    // 写入数据
    for(const auto& result : m_currentResults)
    {
        out << QString("\"%1\",\"%2\",\"%3\",%4,\"%5\",\"%6\",%7,\"%8\",\"%9\"\n")
                   .arg(result.imgName)
                   .arg(result.trueLabel.isEmpty() ? "未知" : result.trueLabel)
                   .arg(result.predictLabel)
                   .arg(result.confidence * 100, 0, 'f', 2)
                   .arg(result.isCorrect() ? "是" : "否")
                   .arg(result.getErrorType())
                   .arg(result.totalProcessingTime)
                   .arg(result.imgPath)
                   .arg(result.timeStamp.toString(Qt::ISODate));
    }
    file.close();
    return true;
}

void BatchValidation::onExportCSVReportBtnClicked()
{
    if(m_currentResults.empty())
    {
        QMessageBox::warning(this, "警告", "没有可导出的数据！");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "导出CSV文件", QString("validation_results_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "CSV文件 (*.csv)");
    if(fileName.isEmpty())
        return;

    if(exportToCSV(fileName))
    {
        QMessageBox::information(this, "成功", "CSV文件导出成功");
        LOG_INFO(QString("CSV文件导出成功：%1").arg(fileName));
    }
    else
        QMessageBox::information(this, "失败", "CSV文件导出失败");
}

void BatchValidation::onGenerateVisualrReportBtnClicked()
{
    if(m_currentResults.isEmpty())
    {
        QMessageBox::warning(this, "警告", "没有可导出的数据！");
        return;
    }

    QString baseDir = QFileDialog::getExistingDirectory(this, "选择报告保存目录", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if(baseDir.isEmpty())
        return;

    m_confusionMatrixPlot->saveAsImage(QString("%1/Confusion_%2.png").arg(baseDir).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
    m_precisionRecallChart->saveAsImage(QString("%1/PrecisionRecall_%2.png").arg(baseDir).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
}

void BatchValidation::onViewErrorSampleBtnClicked()
{

}

void BatchValidation::clearResults()
{
    m_currentResults.clear();

    m_resultsTabel->setRowCount(0);
    m_progressBar->setValue(0);
    m_statusLabel->setText("就绪");

    m_csvBtn->setEnabled(false);
    m_visualizationBtn->setEnabled(false);
    m_errorBtn->setEnabled(false);

    m_hasResults = false;

    m_confusionMatrixPlot->clear();

}

void BatchValidation::updateValidationControls(bool running)
{
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);

    m_csvBtn->setEnabled(!running);
    m_visualizationBtn->setEnabled(!running);
    m_errorBtn->setEnabled(!running);
    m_modelCombo->setEnabled(!running);
}


