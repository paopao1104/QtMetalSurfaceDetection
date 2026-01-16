#include "core/Consumer.h"

#include "core/ImageProcessor.h"
#include "core/FeatureExtractor.h"
#include "core/Classifier.h"

#include "data/DatasetModel.h"
#include <QFileInfo>
#include <QDir>

Consumer::Consumer(TaskQueue* queue, QObject* parent) : QThread(parent)
{
    m_queue = queue;
    qDebug() << "Consumer constructed:" << this;
}

Consumer::~Consumer()
{
    qDebug() << "Consumer destroyed:" << this;
}

void Consumer::run()
{
    ImageProcessor imgProcessor;
    FeatureExtractor featureExtractor;
    Classifier classifier;

    while(true)
    {
        QString imgPath = m_queue->dequeue();
        if(imgPath.isEmpty())
        {
            if(m_queue->isFinished() || m_queue->isStopRequest())
                break;
            else
                continue;
        }

        emit progressUpdated(imgPath);

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
                emit resultReady(result, true);
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

        // 发送单个结果
        emit resultReady(result);
    }
}
