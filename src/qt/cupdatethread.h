#ifndef CUPDATETHREAD_H
#define CUPDATETHREAD_H

#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QObject>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>

class CUpdateThread : public QThread
{
    Q_OBJECT
public:
    CUpdateThread(QObject* parent = 0);
    ~CUpdateThread();

    void stopThread();

    static void ChecketUpdate(bool& needUpdate,QString& version,bool& stopMiner);
    static bool getSoftUpdate(bool& needUpdate,QString& version,bool& stopMiner);
    static bool parseJson(const QByteArray& json,bool& needUpdate,QString& version,bool& stopMiner);
private:
    bool m_stopThread;

    void updateclient(const QString& version,bool stopMiner);
protected:
    void run();

signals:
    void updateClient(const QString& version,bool stopMiner);
    void threadStoped();
};

class CNetwork : public QNetworkAccessManager
{
public:
    CNetwork(QObject* parent=0);

    QByteArray network_get(const QString& url,int msec=6000);
private:

};

#endif // CUPDATETHREAD_H
