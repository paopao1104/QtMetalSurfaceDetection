#include "core/ImageProcessor.h"
#include <QElapsedTimer>
#include <opencv2/opencv.hpp>



cv::Mat ImageProcessor::preprocess(const cv::Mat& input, qint64& processingTime)
{
    QElapsedTimer timer;
    timer.start();

    cv::Mat result;

    // 灰度化
    cv::cvtColor(input, result, cv::COLOR_BGR2GRAY);
    // 统一缩放到64x64
    cv::resize(result, result, cv::Size(64, 64));
    // 滤波
    cv::GaussianBlur(result, result, cv::Size(5,5), 0);
    // 增强对比度
    cv::equalizeHist(result, result);

    processingTime = timer.elapsed();

    return result;
}

cv::Mat ImageProcessor::preprocessWithStats(const cv::Mat& input, ProcessingStats& stats)
{
    qint64 processingTime;
    preprocess(input, processingTime);
    stats.image = input;
    stats.preprocessingTime = processingTime;
    stats.channels = input.channels();
    stats.imageSize = cv::Size(input.cols, input.rows);
    return cv::Mat();
}
