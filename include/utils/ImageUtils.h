#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QImage>
#include <QPixmap>
#include <opencv2/opencv.hpp>

class QLabel;

class ImageUtils
{
public:
    // 禁止实例化，全部为静态函数
    ImageUtils() = delete;
    ~ImageUtils() = delete;

    static QImage matToQImage(const cv::Mat& mat);

    static QPixmap matToPixmap(const cv::Mat& mat, QLabel* label = nullptr);

    static cv::Mat getHOGVisualization(const cv::Mat& preprocessedImg);
};

#endif // IMAGEUTILS_H
