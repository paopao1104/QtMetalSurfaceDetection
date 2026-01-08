#ifndef PRECISIONRECALLBARCHART_H
#define PRECISIONRECALLBARCHART_H

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

struct ClassMetric
{
    QString name;
    double precision;
    double recall;
    double f1Score;
    int support;
};

class PrecisionRecallBarChart : public QWidget
{
    Q_OBJECT
public:
    explicit PrecisionRecallBarChart(QWidget *parent = nullptr);

    void setData(const QVector<ClassMetric>& metrics);
    void updateChart();
    void setShowF1Score(bool show);
    void setTargetThreshold(double threshold);
    void exportChart(const QString& filePath);

private:
    QChart* m_chart;
    QChartView* m_chartView;
    QVector<ClassMetric> m_metrics;

    bool m_showF1Score;
    double m_targetThreshold;

    void setupChart();
    void addThresholdLine();
    void createCustomToolTips();
    void createGroupedChart(const QVector<ClassMetric>& metrics);
};

#endif // PRECISIONRECALLBARCHART_H
