#ifndef CLASSIFIER_H
#define CLASSIFIER_H
#include "core/PredictionResult.h"

#include <QObject>
#include <QDateTime>
#include <opencv2/opencv.hpp>

struct ClassifyStats
{
    qint64 classifyTime;   // 分类时间
    DefectType classId;    // 分类结果的类Id
    QString label;         // 分类结果的标签
    double confidence;     // 置信度
};

class Classifier : public QObject
{
    Q_OBJECT
public:
    static void classify(const std::vector<float>& features, ClassifyStats& stats);

    // static PredictionResult classify(const std::vector<float>& features, qint64& classifierTime);
};

#endif
