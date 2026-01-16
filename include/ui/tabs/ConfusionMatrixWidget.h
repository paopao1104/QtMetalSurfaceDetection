#ifndef CONFUSIONMATRIXWIDGET_H
#define CONFUSIONMATRIXWIDGET_H

#include <QMap>
#include <QWidget>

#include "core/PredictionResult.h"
class QCustomPlot;

struct ConfusionMatrix
{
    int truePositives;  // 真阳性
    int trueNegatives;  // 真阴性
    int falsePositives; // 假阳性
    int falseNegatives; // 假阴性
    QMap<QString, QMap<QString, int>> matrix; // 多类别混淆矩阵
};

class ConfusionMatrixWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfusionMatrixWidget(QWidget* parent = nullptr);

    void setMatrixData(const QVector<QVector<int>>& confusionMatrix);
    void setCategoryNames(QStringList names);
    void setData(const QVector<PredictionResult>& results);
    void saveAsImage(const QString& filePath);
    void clear();
private:
    void calculateMatrixs(const QVector<PredictionResult>& results);
    void setupPlot();

private:
    QCustomPlot* m_customPlot;

    QVector<QVector<int>> m_confusionMatrix; // 混淆矩阵数据
    QStringList m_categoryNames;   // 类别名称
    int m_classCount;              // 类别数量
};


#endif // CONFUSIONMATRIXWIDGET_H
