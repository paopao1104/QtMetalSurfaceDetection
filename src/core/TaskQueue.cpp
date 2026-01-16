#include "core/TaskQueue.h"
#include <QString>
#include <QMutexLocker>
#include <QThread>
#include <QDebug>

TaskQueue::TaskQueue() : m_isFinished(false), m_stopRequest(false)
{}

void TaskQueue::enqueue(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    if(m_stopRequest)
        return;

    m_pathsQueue.enqueue(path);
    m_condition.wakeOne();
}

QString TaskQueue::dequeue()
{
    QMutexLocker locker(&m_mutex);
    while(m_pathsQueue.empty() && !m_isFinished && !m_stopRequest)
    {
        m_condition.wait(&m_mutex);
    }
    if(m_stopRequest  || (m_pathsQueue.empty() && m_isFinished))
        return QString();
    return m_pathsQueue.dequeue();
}

void TaskQueue::setFinished()
{
    QMutexLocker locker(&m_mutex);
    m_isFinished = true;
    m_condition.wakeAll();
}

bool TaskQueue::isFinished() const
{
    QMutexLocker locker(&m_mutex);
    return m_pathsQueue.empty() && m_isFinished;
}

void TaskQueue::setStopRequest()
{
    QMutexLocker locker(&m_mutex);
    m_stopRequest = true;
    m_condition.wakeAll();
}

bool TaskQueue::isStopRequest() const
{
    QMutexLocker locker(&m_mutex);
    return m_stopRequest;
}
