#include "utils/ImageUtils.h"
#include <QLabel>
using namespace cv;

QImage ImageUtils::matToQImage(const cv::Mat& mat)
{
    QImage qImg;
    Mat result;
    if(mat.type() == CV_8UC3)
    {
        cvtColor(mat, result, cv::COLOR_BGR2RGB);
        qImg = QImage(result.data, result.cols, result.rows,
                      static_cast<int>(result.step), QImage::Format_RGB888).copy();
    }
    else if(mat.type() == CV_8UC1)
    {
        cvtColor(mat, result, cv::COLOR_BGR2GRAY);
        qImg = QImage(result.data, result.cols, result.rows,
                      result.step, QImage::Format_Grayscale8).copy();
    }
    return qImg;
}

QPixmap ImageUtils::matToPixmap(const cv::Mat& mat, QLabel* label)
{
    QImage qImg;
    if(mat.channels() == 1)
        qImg = QImage(mat.data, mat.cols, mat.rows,
                         mat.step, QImage::Format_Grayscale8).copy();
    else
        qImg = matToQImage(mat);

    if(label == nullptr)
        return QPixmap::fromImage(qImg);
    return QPixmap::fromImage(qImg).scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

cv::Mat ImageUtils::getHOGVisualization(const cv::Mat& preprocessedImg) {
    // 1. 确保图像尺寸符合HOG要求
    cv::Mat img;
    if (preprocessedImg.size() != cv::Size(64, 64)) {
        cv::resize(preprocessedImg, img, cv::Size(64, 64));
    } else {
        img = preprocessedImg.clone();
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
