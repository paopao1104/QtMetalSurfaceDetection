#ifndef PRECISIONRECALLBARCHART_H
#define PRECISIONRECALLBARCHART_H

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include "core/PredictionResult.h"

struct ClassMetric
{
    QString name;
    double precision;
    double recall;
    double f1Score;
    int support;
    int truePositives;  // 真阳性
    int trueNegatives;  // 真阴性
    int falsePositives; // 假阳性
    int falseNegatives; // 假阴性
};

class PrecisionRecallBarChart : public QWidget
{
    Q_OBJECT
public:
    explicit PrecisionRecallBarChart(QWidget *parent = nullptr);

    void setData(const QVector<PredictionResult>& results);
    void updateChart();
    void setShowF1Score(bool show);
    void setTargetThreshold(double threshold);
    void exportChart(const QString& filePath);

private:
    void setupChart();
    void addThresholdLine();
    void createCustomToolTips();
    void computeClassMetrics(const QVector<PredictionResult>& results);
private:
    QChart* m_chart;
    QChartView* m_chartView;
    QVector<ClassMetric> m_metrics;

    bool m_showF1Score;
    double m_targetThreshold;

    QStringList m_classLabels;

};

#endif // PRECISIONRECALLBARCHART_H
