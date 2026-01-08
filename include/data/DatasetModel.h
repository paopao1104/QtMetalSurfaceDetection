#ifndef DATASETMODEL_H
#define DATASETMODEL_H

// 模型加载
#include <QObject>
#include <QMutex>
#include <opencv2/opencv.hpp>

class DatasetModel : public QObject
{
    Q_OBJECT

public:
    // 删除拷贝构造和赋值函数，确保单例
    DatasetModel(const DatasetModel& fileName) = delete;
    DatasetModel& operator=(const DatasetModel&) = delete;

    static DatasetModel* instance();

    bool loadModel(const QString& modelPath);

    float predict(const cv::Mat& featureVector);

    bool isLoaded() const;

    QString getModelPath() const;

    bool reload();

    QString getModelType() const;

    QVariantMap getModelParameters() const;

    double getConfidence(const cv::Mat& featureVector) const;

private:
    explicit DatasetModel(QObject* parent = nullptr);

    static DatasetModel* m_instance;

    static QMutex m_mutex;

    cv::Ptr<cv::ml::SVM> m_model;

    QString m_modelPath;

    bool m_isLoaded;
};

#define DATASET_MODEL DatasetModel::getInstance()

#endif
