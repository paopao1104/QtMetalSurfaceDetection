#include "utils/TimerUtils.h"


QElapsedTimer TimerUtils::startTimer()
{
    QElapsedTimer timer;
    timer.start();
    return timer;
}

qint64 TimerUtils::elapsedTime(const QElapsedTimer& timer)
{
    return timer.elapsed();
}

QString TimerUtils::formatTime(qint64 milliseconds)
{
    if(milliseconds < 1000)
    {
        return QString("%1 ms").arg(milliseconds);
    }
    else if(milliseconds < 60000)
    {
        return QString("%1.%2 s").arg(milliseconds/1000).arg((milliseconds % 1000) / 100);
    }
    else
    {
        int minutes = milliseconds / 60000;
        int seconds = (milliseconds % 60000) / 1000;
        return QString("%!:%2").arg(minutes).arg(seconds,2,10,QLatin1Char('0'));
    }
}
