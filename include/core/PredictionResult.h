#ifndef PREDICTIONRESULT_H
#define PREDICTIONRESULT_H

#include <QString>
#include <opencv2/opencv.hpp>
#include <QDateTime>

struct PredictionResult
{
    QString imgName;
    QString imgPath;
    cv::Size imgSize;
    qint64 fileSize;

    QDateTime timeStamp;    // 预测开始的时间戳
    QString predictLabel;   // 预测标签
    QString error;          // 错误信息
    double confidence;      // 置信度
    int predictClassId;     // 预测类别Id

    bool processingSuccess;
    QString errorMessage;
    QString trueLabel;      // 真实标签
    int trueClassId;        // 真实类Id

    QString featureDimension;
    QString featureType;

    qint64 totalProcessingTime;
    QString modelName;
    QString modelPath;

};

#endif // PREDICTIONRESULT_H
