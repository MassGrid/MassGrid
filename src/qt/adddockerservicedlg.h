#ifndef ADDDOCKERSERVICEDLG_H
#define ADDDOCKERSERVICEDLG_H

#include <QDialog>
#include <QMouseEvent>
#include <QObject>
#include "amount.h"
#include "networkers.h"

namespace Ui {
class AddDockerServiceDlg;
}

class WalletModel;
class SendCoinsRecipient;
class DockerCreateService;
class ResourceItem;

class CheckoutTransaction;
class AskDNDataWorker;
class LoadingWin;
class AddDockerServiceDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AddDockerServiceDlg(QWidget *parent = nullptr);
    ~AddDockerServiceDlg();

    void setaddr_port(const std::string& addr_poprt);
    void settxid(const std::string& txid);

    void setWalletModel(WalletModel* walletmodel);

    bool isTransactionFinished(std::string& strErr);

private:
    Ui::AddDockerServiceDlg *ui;

    QPoint m_last;
    bool m_mousePress;
    std::string m_addr_port;
    std::string m_txid;
    std::string m_masterndoeAddr;
    WalletModel *m_walletModel;
    // DockerCreateService m_createService;
    CheckoutTransaction *m_checkoutTransaction;
    AskDNDataWorker *m_askDNDataWorker;
    LoadingWin *m_loadingWin;
    // CAmount m_amount;
    DockerCreateService *m_createService;
    CAmount m_amount;

private:
    bool createDockerService();

    void doStep1(ResourceItem*);

    bool sendCoin();
    bool validate(SendCoinsRecipient&);
    void loadResourceData();
    void checkCreateTaskStatus(std::string txid);
    bool deleteService(const std::string& strServiceid);
    void gotoStep1Page();
    QString getErrorMsg(int errCode);
    void askForDNData();
    void showLoading(const QString & msg);
    void hideLoadingWin();
    void filterResource(std::string);
    void initCombobox();
    void startCheckTransactionWork();
    void stopAndDelTransactionThread();
    void resetGUITimer();
    void expansionResourceItems();
    void startAskDNDataWork(const char* slotMethod, bool needAsk = true);
    // void saveServiceData(ServiceInfo serviceInfo);
    
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    void resizeEvent(QResizeEvent *event);

private Q_SLOTS:
    void slot_openPubKeyFile();
    void doStep2();
    void doStep3();
    void doTransaction();
    void doStep4();
    void slot_nextStep();
    void slot_close();

    void refreshServerList();
    void updateServiceListFinished(bool);
    void initTableWidget();
    void slot_buyClicked();

    void slot_refreshTransactionStatus();
    void transactionFinished();
    void updateDNDataAfterCreate(bool);
    void slot_updateTaskTime(int);
    void updateCreateServerWaitTimer(int);
    void slot_hireTimeChanged(int);
    void slot_gpuComboxCurrentIndexChanged(int);
    void slot_searchMinAmount();
    void slot_minAmounttextChanged(QString);
    void slot_autominerChecked(bool);
    void slot_setFocus();
};

class CheckoutTransaction : public QObject
{
    Q_OBJECT
public:
    explicit CheckoutTransaction(std::string txid,QObject* parent = nullptr);
    ~CheckoutTransaction();
    static bool isTransactionFinished(std::string txid,std::string& strErr);

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

// class AskDNDataWorker : public QObject
// {
//     Q_OBJECT
// public:
//     explicit AskDNDataWorker(QObject* parent = nullptr);
//     ~AskDNDataWorker();

//     bool isNeedToWork() { return m_isNeedToWork; };
//     void setNeedToWork(bool flag) { m_isNeedToWork = flag;};

//     bool isAskDNDataFinished();

// private:
//     bool m_isNeedToWork;

// Q_SIGNALS:
//     void askDNDataFinished(bool isTimeOut);
//     void askDNDataTimeout();
//     void updateTaskTime(int);
//     void threadStopped();
// private Q_SLOTS:
//     void startTask();
// };

#endif // ADDDOCKERSERVICEDLG_H