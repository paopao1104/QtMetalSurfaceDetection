#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

class QString;

class TaskQueue
{
public:
    TaskQueue();

    void enqueue(const QString& path);
    QString dequeue();
    void setFinished();
    bool isFinished() const;

    void setStopRequest();
    bool isStopRequest() const;

private:
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<QString> m_pathsQueue;
    bool m_isFinished;
    bool m_stopRequest;
};

#endif // TASKQUEUE_H
