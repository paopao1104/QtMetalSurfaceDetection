#ifndef BATCHVALIDATION_H
#define BATCHVALIDATION_H
#include <QWidget>
#include <QMutex>
#include <QDateTime>
#include "core/PredictionResult.h"

class MetalSurfaceDetection;
class QRadioButton;
class QPushButton;
class QLabel;
class QComboBox;
class QFileSystemModel;
class QTableWidget;
class QChartView;
class QProgressBar;
class QThread;
class ConfusionMatrixWidget;
class PrecisionRecallBarChart;

struct PredictionResult;

/**
 * @brief 批量验证工作线程
 */
class BatchValidationWorker : public QObject
{
    Q_OBJECT
public:
    explicit BatchValidationWorker(QObject* parent = nullptr);

public slots:
    void  startBatchValidation(const QString& datasetPath, const QString& modelPath, const QString& datasetType);
    void stopBatchValidation();

signals:
    void progressUpdated(int current, int total, const QString& curretFile);
    void singleResultReady(const PredictionResult& result);
    void batchCompleted(const QVector<PredictionResult>& results);
    void validationStopped();
    void errorOccurred(const QString& error);

private:
    bool m_stopRequested;
    QMutex m_mutex;
};

/**
 * @brief 批量验证与报告标签页
 */
class BatchValidation : public QWidget
{
    Q_OBJECT

public:
    explicit BatchValidation(MetalSurfaceDetection* parentWindow, QWidget *parent = nullptr);
    ~BatchValidation();
    void setRootPath(const QString& rootPath);

public slots:
    void onRadioBtnClicked();
    void onStartBatchValidation();
    void onStopBatchValidation();
    void handleProgressUpdate(int current, int total, const QString& curretFile);
    void handleSingleResult(const PredictionResult& result);
    void handleBatchCompleted(const QVector<PredictionResult>& results);
    void handleErrorOccurred(const QString& error);

    void onExportCSVReportBtnClicked();
    void onGenerateVisualrReportBtnClicked();
    void onViewErrorSampleBtnClicked();
private:
    void setupUI();
    void setupComponents();
    void setupCharts();
    void setupTable();
    void setupWokerThread();
    void setupConnection();
    void clearResults();
    void updateValidationControls(bool running);

    void addResultToTable(const PredictionResult& result);
    void updateConfusionMatrixPlot();
    void updatePrecisionRecallChart();

private:
    MetalSurfaceDetection* m_parentWindow;

    QRadioButton* m_validRadioBtn;
    QRadioButton* m_testRadioBtn;

    QComboBox* m_modelCombo;
    QString m_rootPath;

    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;

    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    QPushButton* m_csvBtn;
    QPushButton* m_visualizationBtn;
    QPushButton* m_errorBtn;

    QTableWidget* m_resultsTabel;
    ConfusionMatrixWidget* m_confusionMatrixPlot;
    PrecisionRecallBarChart* m_precisionRecallChart;

    QString m_datasetPath;
    QStringList m_modelNameList;
    QString m_modelsPath;
    bool m_loadFlag{false};

    QChartView* m_confusionChart;
    QChartView* m_prChart;

    bool m_hasDataset{false};

    BatchValidationWorker* m_worker;
    QThread* m_workerThread;
    bool m_isValidating{false};
    bool m_hasResults{false};
    QDateTime m_validationStartTime;

    QVector<PredictionResult> m_currentResults;
    QVector<PredictionResult> m_filteredResults;
};

#endif
