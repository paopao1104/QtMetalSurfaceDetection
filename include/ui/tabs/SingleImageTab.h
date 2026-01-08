#ifndef SINGLEIMAGE_H
#define SINGLEIMAGE_H
#include <QWidget>
#include <opencv2/opencv.hpp>

class MetalSurfaceDetection;
class QLabel;
class QFileSystemModel;
class QSplitter;
class QPushButton;
class QCheckBox;
class QTextEdit;
class QSlider;
struct ExtractHOGStats;
struct ClassifyStats;

class SingleImage : public QWidget
{
    Q_OBJECT

public:
    explicit SingleImage(MetalSurfaceDetection* parentWindow, QWidget *parent = nullptr);
    ~SingleImage();

    void setFileSystemModel(QFileSystemModel* model);
private:
    void setupUI();
    void setupConnection();
    void setupModelInfo();
    void updateDetectionResult(qint64 totalTime, qint64 preprocessTime, const ExtractHOGStats& hog, const ClassifyStats& result);
    void displayImages(const cv::Mat& preprocessedMat, const cv::Mat& hogVisualization);

signals:
    void sendFileInfo(const QString& fileInfo);

public slots:
    void onTreeViewClicked(const QModelIndex &index);
    void setupSplitter();
    void onStartBtnClicked();
    void resetResultInfo();
    void onAutoCheckBoxChanged();
    void onBrightSliderChanged(int value);
    void onContrastSliderChanged(int value);
private:
    MetalSurfaceDetection* m_parentWindow;

    cv::Mat m_originMat;
    cv::Mat m_oriDisplayMat;

    QLabel* m_originImgLabel;
    QLabel* m_preprocessImgLabel;
    QLabel* m_HOGImgLabel;

    QFileSystemModel* m_fileModel;
    QSplitter* m_splitter;

    QTextEdit* m_logTextEdit;

    QPushButton* m_saveBtn;
    QPushButton* m_changeBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_startBtn;
    QCheckBox*   m_autoCheckBox;
    bool m_isAuto{false};

    QLabel* m_resultL; // 预测结果
    QLabel* m_confidenceL; // 置信度
    QLabel* m_modelStrL; // 模型
    QLabel* m_featureDiL; // 特征维度
    QLabel* m_timeL; // 处理耗时

    QSlider* m_brightSlider;
    QSlider* m_contractSlider;
    QLabel* m_brightValue;
    QLabel* m_contractValue;
};

#endif
