#ifndef METALSURFACEDETECTION_H
#define METALSURFACEDETECTION_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MetalSurfaceDetection;
}
QT_END_NAMESPACE

// === 4个标签页 === //
class SingleImage;
class BatchValidation;

class QFileSystemModel;
class QTreeView;
class QLabel;
class QSplitter;
class QPushButton;

class MetalSurfaceDetection : public QWidget
{
    Q_OBJECT

public:
    MetalSurfaceDetection(QWidget *parent = nullptr);
    ~MetalSurfaceDetection();
public:
    void setupUI();
    void setupSplitter();

    QWidget* createLeftPanel();
    QWidget* createTabsPanel();

    QString rootPath() const;

signals:
    void setupTabsSplitter();

public slots:
    void receiveFileInfo(const QString& fileInfo);
    void onChangeDirBtnClicked();

private:
    QFileSystemModel* m_fileModel;
    QTreeView* m_fileTreeView;
    QLabel* m_infoLabel;
    QPushButton* m_changeDirBtn;

    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    QSplitter* m_CenterSliptter;
private:
    SingleImage* m_singleImageTab; // 标签1：单张图片测试
    BatchValidation* m_batchValidationTab; // 标签2：批量验证与测试

    Ui::MetalSurfaceDetection *ui;
};
#endif // METALSURFACEDETECTION_H
