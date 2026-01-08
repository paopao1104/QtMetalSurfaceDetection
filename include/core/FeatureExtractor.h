#ifndef FEATUREEXTRACTOR_H
#define FEATUREEXTRACTOR_H

#include <QObject>
#include <opencv2/opencv.hpp>

struct ExtractHOGStats
{
    qint64 extractionTime = 0;   // 特征提取耗时
    size_t featureDimension = 0; // 特征维度
    size_t descriptorSize = 0 ;  // 描述子大小
    cv::Mat visualizationl;      // 可视化图像
};

class FeatureExtractor : public QObject
{
    Q_OBJECT
public:
    static std::vector<float> extractHOG(const cv::Mat& input, ExtractHOGStats& extractStats);

    static cv::Mat visualizeHOG(const cv::Mat& input,
                                const std::vector<float>& stats);
};

#endif
