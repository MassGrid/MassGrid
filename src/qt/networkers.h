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
    void updateTaskTime(int);
    void threadStopped();
private Q_SLOTS:
    void startTask();
};

class AskServicesWorker : public QObject
{
    Q_OBJECT
public:
    explicit AskServicesWorker(QObject* parent = nullptr);
    ~AskServicesWorker();

    bool isNeedToWork() { return m_isNeedToWork; };
    void setNeedToWork(bool flag) { m_isNeedToWork = flag;};

    bool isAskServicesFinished();

private:
    bool m_isNeedToWork;

Q_SIGNALS:
    void askServicesFinished(bool isTimeOut);
    void askServicesTimeout();
    void updateTaskTime(int);
    void threadStopped();
private Q_SLOTS:
    void startTask();
};

class CheckoutTransaction : public QObject
{
    Q_OBJECT
public:
    explicit CheckoutTransaction(std::string txid,QObject* parent = nullptr);
    ~CheckoutTransaction();
    bool isTransactionFinished(std::string txid,std::string& strErr);

    bool isNeedToWork() { return m_isNeedToWork; };
    void setNeedToWork(bool flag) { m_isNeedToWork = flag;};

private:
    std::string m_txid;
    bool m_isNeedToWork;

Q_SIGNALS:
    void checkTransactionFinished();
    void checkTransactionTimeOut();
    void updateTaskTime(int);
    void threadStopped();

private Q_SLOTS:
    void startTask();

};

#endif // NETWORKERS_H