#include "core/Classifier.h"
#include "data/DatasetModel.h"

#include <QElapsedTimer>

void Classifier::classify(const std::vector<float>& features, ClassifyStats& stats)
{
    QElapsedTimer timer;
    timer.start();

    // 将特征向量转换为cv::Mat
    cv::Mat featureMat(1, static_cast<int>(features.size()), CV_32FC1);
    for (size_t i = 0; i < features.size(); i++) {
        featureMat.at<float>(0, static_cast<int>(i)) = features[i];
    }

    DatasetModel* instance = DatasetModel::instance();

    float prediction = instance->predict(featureMat);
    int classId = static_cast<int>(prediction);

    QString label;
    if(classId <= 0)
        label = "UNKNOWN";
    else if(classId == 1)
        label = "Crazing";
    else if(classId == 2)
        label = "Inclusion";
    else if(classId == 3)
        label = "Patches";
    else if(classId == 4)
        label = "Pitted";
    else if(classId == 5)
        label = "Rolled";
    else if(classId == 6)
        label = "Scratches";

    stats.label = label;
    stats.classId = static_cast<DefectType>(classId);
    stats.confidence = instance->getConfidence(featureMat);
    stats.classifyTime = timer.elapsed();
}

// PredictionResult Classifier::classify(const std::vector<float>& features, qint64& classifierTime)
// {
//     QElapsedTimer timer;
//     timer.start();

//     PredictionResult result;

//     // 将特征向量转换为cv::Mat
//     cv::Mat featureMat(1, static_cast<int>(features.size()), CV_32FC1);
//     for (size_t i = 0; i < features.size(); i++) {
//         featureMat.at<float>(0, static_cast<int>(i)) = features[i];
//     }

//     DatasetModel* instance = DatasetModel::instance();

//     float prediction = instance->predict(featureMat);
//     int classId = static_cast<int>(prediction);

//     QString label;
//     if(classId <= 0)
//         label = "UNKNOWN";
//     else if(classId == 1)
//         label = "Crazing";
//     else if(classId == 2)
//         label = "Inclusion";
//     else if(classId == 3)
//         label = "Patches";
//     else if(classId == 4)
//         label = "Pitted";
//     else if(classId == 5)
//         label = "Rolled";
//     else if(classId == 6)
//         label = "Scratch";

//     result.predictLabel = label;
//     result.predictClassId = classId;
//     result.confidence = instance->getConfidence(featureMat);


//     classifierTime = timer.elapsed();

//     return result;
// }
