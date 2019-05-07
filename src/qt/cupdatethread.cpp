#include "cupdatethread.h"
#include <QDateTime>
#include <QTimer>
#include <QUrlQuery>
#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>

#define CHECKOUTURL "https://www.massgrid.com/assets/download/pcupdate.json"

CUpdateThread::CUpdateThread(QObject* parent)
    :QThread(parent)
    ,m_stopThread(false)
{
}

CUpdateThread::~CUpdateThread()
{

}

void CUpdateThread::stopThread()
{
    m_stopThread = true;
}

void CUpdateThread::ChecketUpdate(bool& needUpdate,QString& version,bool& stopMiner)
{
    while(!getSoftUpdate(needUpdate, version,stopMiner));
}

bool CUpdateThread::getImages(QStringList& images)
{
    int count = 0;
    while(!getImagesData(images)){
        if(count++ > 3){
            return false;
        }
        else{
            return true;
        }
    }
}

bool CUpdateThread::getImagesData(QStringList& images)
{
    CNetwork network;
    QByteArray retStr=network.network_get(CHECKOUTURL);

    bool parseFlag = parseImageJson(retStr,images);
    return parseFlag;
}

void CUpdateThread::run()
{
    QString curTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString hour = curTime.split(":").first();
    QString min = curTime.split(":").at(1);

    QString version;
    bool stopMiner = false;
    bool needUpdate = false;

    while(!getSoftUpdate(needUpdate, version,stopMiner)){
        if(m_stopThread){
            Q_EMIT threadStoped();
            return ;
        }
    }

    if(needUpdate)
        Q_EMIT updateClient(version,stopMiner);

    int firsttime = (23 - hour.toInt())*3600;
    firsttime+=(60 - min.toInt())*60;

    while(--firsttime){
        QThread::sleep(1);
        if(m_stopThread){
            Q_EMIT threadStoped();
            return ;
        }
    }

    while(true){
        while(!getSoftUpdate(needUpdate, version,stopMiner)){
            if(m_stopThread){
                Q_EMIT threadStoped();
                return ;
            }
        }
        if(needUpdate)
            Q_EMIT updateClient(version,stopMiner);

        int timecount = 24*3600;
        while(--timecount){
            if(m_stopThread){
                Q_EMIT threadStoped();
                return ;
            }
            QThread::sleep(1);
        }
    }
}

bool CUpdateThread::getSoftUpdate(bool& needUpdate,QString& version,bool& stopMiner)
{
    CNetwork network;
    QByteArray retStr=network.network_get(CHECKOUTURL);

    bool parseFlag = parseJson(retStr,needUpdate,version,stopMiner);

    return parseFlag;
}

//{
//    "name": "MassGrid",
//    "needUpdate": false,
//    "info": {
//        "version": "1.1.0",
//        "stopMining": true
//    }
//}

bool CUpdateThread::parseJson(const QByteArray &json,bool& needUpdate,QString& version,bool& stopMiner)
{
    QJsonParseError jsonError;
    QJsonDocument docment = QJsonDocument::fromJson(json, &jsonError);

    if(docment.isNull() || !docment.isObject()){
        return false;
    }

    QJsonObject doc_object = docment.object();
    QJsonObject info_object = doc_object.value("info").toObject();
    needUpdate = doc_object.value("needUpdate").toBool();
    QString versionStr = info_object.value("version").toString();

    if(versionStr.isEmpty()){
        return false;
    }
    else{
        bool flag = info_object.value("stopMining").toBool();
        version = versionStr;
        stopMiner = flag;
        return true;
    }
    //error:data is null
    return false;
}

bool CUpdateThread::parseImageJson(const QByteArray &json,QStringList& images)
{
    QJsonParseError jsonError;
    QJsonDocument docment = QJsonDocument::fromJson(json, &jsonError);

    if(docment.isNull() || !docment.isObject()){
        return false;
    }

    QJsonObject doc_object = docment.object();
    // QJsonObject info_object = doc_object.value("info").toObject();
    // needUpdate = doc_object.value("needUpdate").toBool();
    // QString versionStr = info_object.value("version").toString();
    QJsonArray images_array = doc_object.value("images").toArray();

    if(images_array.isEmpty()){
        return false;
    }
    else{
        int size = images_array.size();
        for(int i=0;i<size;i++){
            images.append(images_array.at(i).toString());
        }
        
        return true;
    }

    // if(versionStr.isEmpty()){
    //     return false;
    // }
    // else{
    //     bool flag = info_object.value("stopMining").toBool();
    //     version = versionStr;
    //     stopMiner = flag;
    //     return true;
    // }
    //error:data is null
    return false;
}

void CUpdateThread::updateclient(const QString& version,bool stopMiner)
{
    Q_EMIT updateClient(version,stopMiner);
}

CNetwork::CNetwork(QObject *parent)
{
    
}

QByteArray CNetwork::network_get(const QString &url,int msec)
{
    QByteArray retStr;
    QNetworkRequest request(url);

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(msec);
    QEventLoop loop;
    connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
    connect(this,SIGNAL(finished(QNetworkReply*)),&loop,SLOT(quit()));

    QNetworkReply *m_reply=get(request);
    timer.start();
    loop.exec();

    if(timer.isActive()){
        timer.stop();
        int error=m_reply->error();
        if(error>0){
            //error
        }
        else{
            int v = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (v >= 200 && v < 300) {
                // Success
                retStr=m_reply->readAll();
            }
        }
    }
    else{
        //timeout
        disconnect(this,SIGNAL(finished(QNetworkReply*)),&loop,SLOT(quit()));
        m_reply->abort();
    }
    return retStr;
}
