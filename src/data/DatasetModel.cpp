#include "data/DatasetModel.h"
#include <QDebug>
#include <QFile>
#include <QVariantMap>

DatasetModel* DatasetModel::m_instance = nullptr;
QMutex DatasetModel::m_mutex;

DatasetModel::DatasetModel(QObject* parent)
    :QObject(nullptr)
    ,m_isLoaded(false)
{
    // 创建SVM对象
    m_model = cv::ml::SVM::create();
}

DatasetModel* DatasetModel::instance()
{
    QMutexLocker locker(&m_mutex);

    if(!m_instance)
    {
        m_instance = new DatasetModel();
        qDebug() << "ModelManager单例已创建";
    }
    return m_instance;
}

bool DatasetModel::loadModel(const QString& modelPath)
{
    QMutexLocker locker(&m_mutex);

    if(!QFile::exists(modelPath))
    {
        qDebug() << "模型文件不存在！" << modelPath;
        return false;
    }

    try
    {
        // 使用opencv加载模型
        m_model = cv::ml::SVM::load(modelPath.toStdString());
        if(m_model.empty())
        {
            qWarning() << "模型加载失败，文件可能已损坏！";
            m_isLoaded = false;
            return false;
        }

        m_modelPath = modelPath;
        m_isLoaded = true;

        qDebug() << "模型加载成功:" << modelPath;
        return true;
    }
    catch(const cv::Exception& e)
    {
        qWarning() << "opencv异常：" << e.what();
        m_isLoaded = false;
        return false;
    }
    return m_isLoaded;
}

float DatasetModel::predict(const cv::Mat& featureVector)
{
    QMutexLocker locker(&m_mutex);

    if(!m_isLoaded)
    {
        qWarning() << "模型未加载，无法预测";
        return -1.0f;
    }

    try {
        // 确保特征向量格式正确
        if(featureVector.empty())
        {
            qWarning() << "特征向量为空";
            return -1.0f;
        }

        // 将特征向量转换为浮点数
        //cv::Mat floatFeatures;
        //featureVector.convertTo(floatFeatures, CV_32F);
        // 进行预测
        float prediction = m_model->predict(featureVector);
        return prediction;
    } catch (const cv::Exception& e) {
        qWarning() << "预测时发生opengcv异常：" << e.what();
        return -1.0f;
    }
}

bool DatasetModel::isLoaded() const
{
    return m_isLoaded;
}

QString DatasetModel::getModelPath() const
{
    return m_modelPath;
}

bool DatasetModel::reload()
{
    if (m_modelPath.isEmpty()) {
        qWarning() << "没有可重新加载的模型路径";
        return false;
    }

    return loadModel(m_modelPath);
}

QString DatasetModel::getModelType() const
{
    if(!m_isLoaded) return "模型未加载";

    // 从文件路径或模型本身获取类型
    if(m_modelPath.contains("svm_brf")) return "svm_brf";
    if(m_modelPath.contains("svm_linear")) return "svm_linear";
    if(m_modelPath.contains("svm_cnn")) return "svm_cnn";

    // 尝试从OpenCV模型中获取类型
    if(m_model)
    {
        int type = m_model->getType();
        if(type == cv::ml::SVM::C_SVC)
        {
            int kernelType = m_model->getKernelType();
            if(kernelType == cv::ml::SVM::RBF) return "svm_brf";
            if(kernelType == cv::ml::SVM::LINEAR) return "svm_linear";
        }
    }

    return "未知";
}

QVariantMap DatasetModel::getModelParameters() const
{
    QVariantMap params;

    if(m_model)
    {
        params["C"] = m_model->getC();
        params["gamma"] = m_model->getGamma();
        params["type"] = m_model->getType();
        params["kernel"] = m_model->getKernelType();
    }

    return params;
}

double DatasetModel::getConfidence(const cv::Mat& featureVector) const
{
    if(!m_isLoaded || featureVector.empty())
        return 0.0;

    try {
        cv::Mat floatFeatures;
        featureVector.convertTo(floatFeatures, CV_32FC1);
        // 获取决策函数值
        float decisionValue = m_model->predict(floatFeatures);
        double confidence = 1.0 / (1.0 + exp(-decisionValue));

        // 限制在0-1之间
        confidence = qMax(0.0, qMin(1.0, confidence));
        return confidence;

    } catch (const cv::Exception& e) {
        qWarning() << "获取置信度失败：" << e.what();
        return 0.0;
    }
}


// void DataLoader::Preprocess()
// {
//     // 色彩空间转换
//     cvtColor(m_img, preprocessImg, COLOR_RGB2GRAY);

//     // 尺寸归一化
//     resize(preprocessImg, preprocessImg, cv::Size(64, 64)); // 统一缩放到64x64

//     // 图像增强
//     // 高斯去噪
//     GaussianBlur(preprocessImg, preprocessImg, cv::Size(5,5), 0);
//     // 直方图增加对比度
//     equalizeHist(preprocessImg,preprocessImg);

//     m_qPreprocessImage = QImage(preprocessImg.data, preprocessImg.cols, preprocessImg.rows,
//                                 preprocessImg.step, QImage::Format_Grayscale8).copy();
// }

// void DataLoader::extractHOGFeatures()
// {
//     HOGDescriptor hog(Size(64,64), Size(16,16),Size(8,8),Size(8,8),9);
//     std::vector<float> descriptors;
//     hog.compute(preprocessImg, descriptors);
//     Mat featureMat(1, descriptors.size(), CV_32FC1, descriptors.data());
//     qDebug() << "特征提取完成！特征向量维度：" << featureMat.cols << "(长度)" ;
// }
