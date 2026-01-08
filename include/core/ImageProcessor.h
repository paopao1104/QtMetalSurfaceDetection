#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <opencv2/opencv.hpp>

struct ProcessingStats
{
    cv::Mat image;                // 原图像矩阵
    cv::Size imageSize;           // 图像尺寸
    int channels = 0;             // 通道数
    double memoryUsage = 0.0;     // 内存使用
    qint64 preprocessingTime = 0; // 预处理耗时
};

namespace cv {
class Mat;
}

class ImageProcessor : public QObject
{
    Q_OBJECT
public:
    static cv::Mat preprocess(const cv::Mat& input, qint64& processingTime);

    static cv::Mat preprocessWithStats(const cv::Mat& input,
                                                ProcessingStats& stats);
};

#endif
