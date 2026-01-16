#include "ui/tabs/ConfusionMatrixWidget.h"
#include "utils/qcustomplot.h"

ConfusionMatrixWidget::ConfusionMatrixWidget(QWidget* parent)
    : QWidget(parent)
    , m_customPlot(new QCustomPlot())
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_customPlot);
}

void ConfusionMatrixWidget::setData(const QVector<PredictionResult>& results)
{
    calculateMatrixs(results);
    setupPlot();
}

void ConfusionMatrixWidget::calculateMatrixs(const QVector<PredictionResult>& results)
{
    m_confusionMatrix.clear();
    m_categoryNames.clear();

    QSet<QString> categoryNames;
    for(int i = 0; i < results.size(); ++i)
    {
        categoryNames.insert(results[i].trueLabel);
        categoryNames.insert(results[i].predictLabel);
    }
    if(!categoryNames.contains("Unknown"))
        categoryNames << "Unknown";

    m_categoryNames = PredictionResult::sortCategoryNames(categoryNames);
    m_classCount = m_categoryNames.size();

    // 初始化混淆矩阵
    m_confusionMatrix.resize(m_classCount);
    for(int i = 0;i < m_classCount; ++i)
    {
        m_confusionMatrix[i].resize(m_classCount);
        for(int j = 0;j < m_classCount; ++j)
        {
            m_confusionMatrix[i][j] = 0;
        }
    }

    for(int i = 0; i < results.size(); ++i)
    {
        int trueClassId = results[i].trueClassId;
        int predictClassId = results[i].predictClassId;

        if((trueClassId >= 0 && trueClassId < m_classCount) &&
            (predictClassId >= 0 && predictClassId < m_classCount))
            m_confusionMatrix[trueClassId][predictClassId]++;
    }
}

void ConfusionMatrixWidget::setupPlot()
{
    m_customPlot->setWindowTitle("混淆矩阵热力图");

    // 准备数据
    int nx = m_classCount; // 类别数
    int ny = m_classCount;

    QVector<QVector<int>> z = m_confusionMatrix;

    // 创建热力图
    QCPColorMap* colorMap = new QCPColorMap(m_customPlot->xAxis, m_customPlot->yAxis);
    colorMap->data()->setSize(nx, ny);
    colorMap->data()->setRange(QCPRange(0, nx - 1), QCPRange(0, ny - 1));

    // 填充数据
    for(int i = 0; i < nx; ++i)
    {
        for(int j = 0; j < ny; ++j)
        {
            colorMap->data()->setCell(i, j, z[i][j]);
        }
    }

    // 设置颜色渐变
    QCPColorGradient gradient;
    gradient.setColorStopAt(0, QColor(255, 255, 255)); // 白色（最小值）
    gradient.setColorStopAt(0.3, QColor(255, 255, 200)); // 浅黄
    gradient.setColorStopAt(0.6, QColor(255, 200, 100)); // 橙色
    gradient.setColorStopAt(1.0, QColor(255, 0, 0)); // 红色（最大值）

    colorMap->setGradient(gradient);
    colorMap->setInterpolate(false); // 不插值，显示块状

    // 添加颜色刻度
    QCPColorScale* colorScale = new QCPColorScale(m_customPlot);
    m_customPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("数量");

    // 设置坐标轴
    m_customPlot->xAxis->setLabel("预测类别");
    m_customPlot->yAxis->setLabel("真实类别");

    // 设置刻度标签为类别名称
    QVector<double> ticks;
    QStringList labels;
    for(int i = 0; i < nx; ++i)
    {
        ticks << i;
        labels << m_categoryNames[i];
    }

    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    textTicker->setTicks(ticks, labels);

    m_customPlot->xAxis->setTicker(textTicker);
    m_customPlot->yAxis->setTicker(textTicker);

    // 调整显示范围
    colorMap->rescaleDataRange(true);

    // 显示数值标签
    for(int i = 0; i < nx; ++i)
    {
        for(int j = 0; j < ny; ++j)
        {
            QCPItemText* textLabel = new QCPItemText(m_customPlot);
            textLabel->position->setCoords(i, j);
            textLabel->setText(QString::number(z[i][j]));
            textLabel->setFont(QFont(font().family(), 8));

            // 根据背景色调整文本颜色，确保可读性
            double value = z[i][j];
            double maxValue = 0;
            for(int x = 0; x < nx; ++x)
            {
                for(int y = 0; y < ny; ++y)
                {
                    if(z[x][y] > maxValue)
                        maxValue = z[x][y];
                }
            }

            // 如果值较大，使用白色文本；否则使用黑色
            if(value > maxValue * 0.5)
                textLabel->setColor(Qt::white);
            else
                textLabel->setColor(Qt::black);

            textLabel->setPositionAlignment(Qt::AlignCenter);
        }
    }

    m_customPlot->replot();
}

void ConfusionMatrixWidget::saveAsImage(const QString& filePath)
{
    m_customPlot->savePng(filePath, 800, 600);
}

void ConfusionMatrixWidget::setMatrixData(const QVector<QVector<int>>& confusionMatrix)
{
    m_confusionMatrix = confusionMatrix;
    setupPlot();
}
void ConfusionMatrixWidget::setCategoryNames(QStringList names)
{
    m_categoryNames = {"Crazing", "Inclusion", "Patches", "Pitted"};
}

void ConfusionMatrixWidget::clear()
{
    m_confusionMatrix.clear();
    m_customPlot->clearPlottables();
    m_categoryNames.clear();
}
