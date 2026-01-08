#ifndef TIMERUTILS_H
#define TIMERUTILS_H

#include <QObject>
#include <QElapsedTimer>

class TimerUtils : public QObject
{
    Q_OBJECT
public:
    TimerUtils() = delete;

    // 使用QElapsedTimer（跨平台）
    static QElapsedTimer startTimer();

    // qint64 是一个64位有符号整数类型,通常用于需要大范围整数计算的场景，比如文件大小、时间戳等
    static qint64 elapsedTime(const QElapsedTimer& timer);

    // 格式化时间显示
    static QString formatTime(qint64 milliseconds);
};

#endif // TIMERUTILS_H
