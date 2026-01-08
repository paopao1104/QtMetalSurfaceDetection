#include "core/FeatureExtractor.h"

#include <QElapsedTimer>

using namespace cv;

std::vector<float> FeatureExtractor::extractHOG(const cv::Mat& input, ExtractHOGStats& extractStats)
{
    QElapsedTimer timer;
    timer.start();

    // 设置HOG参数
    HOGDescriptor hog(Size(64,64), Size(16,16),Size(8,8),Size(8,8),9);

    // 提取特征
    std::vector<float> descriptors;
    hog.compute(input, descriptors);

    // 记录统计信息
    extractStats.extractionTime = timer.elapsed();
    extractStats.featureDimension = descriptors.size();
    extractStats.descriptorSize = hog.getDescriptorSize();

    // 创建特征可视化
    extractStats.visualizationl = visualizeHOG(input, descriptors);

    return descriptors;
}

cv::Mat FeatureExtractor::visualizeHOG(const cv::Mat& input, const std::vector<float>& stats)
{
    // 1. 确保图像尺寸符合HOG要求
    cv::Mat img;
    if (input.size() != cv::Size(64, 64)) {
        cv::resize(input, img, cv::Size(64, 64));
    } else {
        img = input.clone();
    }

    // 2. 计算梯度图像（这是HOG的基础）
    cv::Mat gradX, gradY;
    cv::Sobel(img, gradX, CV_32F, 1, 0, 3); // 水平梯度
    cv::Sobel(img, gradY, CV_32F, 0, 1, 3); // 垂直梯度

    // 3. 计算梯度幅值和角度
    cv::Mat magnitude, angle;
    cv::cartToPolar(gradX, gradY, magnitude, angle, true);

    // 4. 将梯度幅值归一化到0-255范围，便于显示
    cv::normalize(magnitude, magnitude, 0, 255, cv::NORM_MINMAX);
    magnitude.convertTo(magnitude, CV_8UC1);

    // 5. 创建一个彩色可视化图
    cv::Mat colorVis = cv::Mat::zeros(img.size(), CV_8UC3);

    // 根据梯度方向和幅值给图像着色
    for (int y = 0; y < magnitude.rows; y++) {
        for (int x = 0; x < magnitude.cols; x++) {
            float ang = angle.at<float>(y, x);
            float mag = magnitude.at<uchar>(y, x) / 255.0;

            // 将角度和幅值转换为HSV颜色
            int hue = (int)(ang / 2.0); // 角度映射到色调（0-180）
            int saturation = 255;
            int value = (int)(mag * 255);

            cv::Vec3b hsvPixel(hue, saturation, value);
            colorVis.at<cv::Vec3b>(y, x) = hsvPixel;
        }
    }

    // 转换为RGB显示
    cv::Mat rgbVis;
    cv::cvtColor(colorVis, rgbVis, cv::COLOR_HSV2BGR);

    return rgbVis;
}
