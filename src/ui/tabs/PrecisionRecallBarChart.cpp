#include "ui/tabs/PrecisionRecallBarChart.h"

#include <QVBoxLayout>
#include <QStringList>
#include <QBarSet>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QLineSeries>
#include <QPen>
#include <QToolTip>

PrecisionRecallBarChart::PrecisionRecallBarChart(QWidget *parent)
    : QWidget(parent)
    , m_chart(new QChart())
    , m_chartView(new QChartView(m_chart))
    , m_showF1Score(true)
    , m_targetThreshold(0.8)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_chartView);

    setupChart();
}

void PrecisionRecallBarChart::computeClassMetrics(const QVector<PredictionResult>& results)
{
    QVector<ClassMetric> metrics;
    QSet<QString> categoryNames;

    // 从结果中获取类别名称
    for(int i = 0; i < results.size(); ++i)
    {
        categoryNames.insert(results[i].trueLabel);
        categoryNames.insert(results[i].predictLabel);
    }

    for(const auto& className : m_classLabels)
    {
        ClassMetric metric;
        metric.name = className;

        // 计算TP, FP, FN
        int tp = 0, fp = 0, fn = 0, tn = 0;
        for(const auto& result : results)
        {
            if(!result.processingSuccess) return;

            bool isTrueClass = (result.trueLabel == className);
            bool isPredClass = (result.predictLabel == className);

            if(isTrueClass && isPredClass) tp++;
            else if(!isTrueClass && isPredClass) fp++;
            else if(isTrueClass && !isPredClass) fn++;
            else tn++;
        }

        metric.truePositives = tp;
        metric.trueNegatives = tn;
        metric.falsePositives = fp;
        metric.falseNegatives = fn;
        metric.support = tp + fn;

        // 计算精确率
        if(tp + fp > 0)
            metric.precision = static_cast<double>(tp) / (tp + fp);
        else
            metric.precision = 0.0;

        // 计算召回率
        if(tp + fn > 0)
            metric.recall = static_cast<double>(tp) / (tp + fn);
        else
            metric.recall = 0.0;

        // 计算f1分数
        if(metric.precision + metric.recall > 0)
            metric.f1Score = 2 * metric.precision * metric.recall / (metric.precision + metric.recall);
        else
            metric.f1Score = 0.0;

        metrics.append(metric);
    }

    m_classLabels = categoryNames.values();
    m_metrics = metrics;
}

void PrecisionRecallBarChart::setupChart()
{
    m_chart->setTitle("各类别精确率-召回率分析");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    // 设置图标主题
    m_chart->setTheme(QChart::ChartThemeLight);

    // 创建图例
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
}

void PrecisionRecallBarChart::setData(const QVector<PredictionResult>& results)
{
    computeClassMetrics(results);
    updateChart();
}

void PrecisionRecallBarChart::updateChart()
{
    m_chart->removeAllSeries();
    m_chart->removeAxis(m_chart->axisX());
    m_chart->removeAxis(m_chart->axisY());

    // 创建条形集
    QBarSet* precisionSet = new QBarSet("精确率");
    QBarSet* recallSet = new QBarSet("召回率");
    QBarSet* f1Set = nullptr;

    if(m_showF1Score)
        f1Set = new QBarSet("F1分数");

    // 填充数据
    for(const auto& metric : m_metrics)
    {
        *precisionSet << metric.precision * 100; // 转换为百分比
        *recallSet << metric.recall * 100;
        if(f1Set)
            *f1Set << metric.f1Score * 100;
    }

    // 设置条形颜色
    precisionSet->setColor(QColor(65,105,225)); // 蓝色 - 精确率
    recallSet->setColor(QColor(220, 20, 60));   // 红色 - 召回率
    if(f1Set)
        recallSet->setColor(QColor(50, 205, 50));   // 绿色 - F1分数

    // 创建条形系列
    QBarSeries* series = new QBarSeries();
    series->append(precisionSet);
    series->append(recallSet);
    if(f1Set)
        series->append(f1Set);

    // 添加到图表
    m_chart->addSeries(series);

    // 创建类别轴
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(m_classLabels);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 创建数值轴
    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setTitleText("百分百 (%)");
    axisY->setLabelFormat("%.0f%%");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 添加目标阈值线
    addThresholdLine();

    // 添加悬停标志
    createCustomToolTips();
}

void PrecisionRecallBarChart::addThresholdLine()
{
    // 创建目标阈值线
    QLineSeries* thresholdSeries = new QLineSeries();
    thresholdSeries->setName(QString("目标阈值 (%1)").arg(m_targetThreshold * 100));

    // 添加点：从第一个类别到最后一个类别
    thresholdSeries->append(0, m_targetThreshold * 100);
    thresholdSeries->append(m_metrics.size() - 1, m_targetThreshold * 100);

    m_chart->addSeries(thresholdSeries);

    // 设置阈值线样式
    QPen pen(Qt::DashLine);
    pen.setColor(Qt::darkGray);
    pen.setWidth(2);
    thresholdSeries->setPen(pen);
}

void PrecisionRecallBarChart::createCustomToolTips()
{
    // 连接悬停信号
    auto seriesList = m_chart->series();

    for(auto series : seriesList)
    {
        if(auto barSeries = qobject_cast<QBarSeries*>(series))
        {
            connect(barSeries, &QBarSeries::hovered, this, [this](bool status, int index, QBarSet* barSet){
                if(status && index < m_metrics.size())
                {
                    const auto& metric = m_metrics[index];
                    QString tooltip;

                    if(barSet->label() == "精确率")
                        tooltip = QString("<b>%1 - 精确率</b><br>"
                                          "值: %2%</br>").arg(metric.name).arg(metric.precision * 100, 0, 'f', 1);
                    else if(barSet->label() == "召回率")
                        tooltip = QString("<b>%1 - 召回率</b><br>"
                                          "值: %2%</br>").arg(metric.name).arg(metric.recall * 100, 0, 'f', 1);
                    else if(barSet->label() == "F1分数")
                        tooltip = QString("<b>%1 - f1分数</b><br>"
                                          "值: %2%</br>").arg(metric.name).arg(metric.f1Score * 100, 0, 'f', 1);

                    // 显示工具提示
                    QToolTip::showText(QCursor::pos(), tooltip);
                }
            });
        }
    }
}

void PrecisionRecallBarChart::setShowF1Score(bool show)
{
    m_showF1Score = show;
}

void PrecisionRecallBarChart::setTargetThreshold(double threshold)
{
    m_targetThreshold = threshold;
}

void PrecisionRecallBarChart::exportChart(const QString& filePath)
{

}



