#ifndef NETWORKERS_H
#define NETWORKERS_H

#include <QObject>
#include <QThread>

class AskDNDataWorker : public QObject
{
    Q_OBJECT
public:
    explicit AskDNDataWorker(QObject* parent = nullptr);
    ~AskDNDataWorker();

    bool isNeedToWork() { return m_isNeedToWork; };
    void setNeedToWork(bool flag) { m_isNeedToWork = flag;};

    bool isAskDNDataFinished();

private:
    bool m_isNeedToWork;

Q_SIGNALS:
    void askDNDataFinished(bool isTimeOut);
    void askDNDataTimeout();
    void updateTaskTime(int);
    void threadStopped();
private Q_SLOTS:
    void startTask();
};

#endif // NETWORKERS_H