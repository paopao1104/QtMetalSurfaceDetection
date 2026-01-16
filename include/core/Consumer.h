#ifndef CONSUMER_H
#define CONSUMER_H

#include "TaskQueue.h"
#include "PredictionResult.h"

#include <QThread>

class Consumer : public QThread
{
    Q_OBJECT
public:
    Consumer(TaskQueue* queue, QObject* parent = nullptr);
    ~Consumer();

signals:
    void progressUpdated(const QString& imgPath);
    void resultReady(const PredictionResult& result, bool imgEmpty = false);

protected:
    void run();

private:
    TaskQueue* m_queue;
};

#endif // CONSUMER_H
