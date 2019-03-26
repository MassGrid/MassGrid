#ifndef ADDDOCKERSERVICEDLG_H
#define ADDDOCKERSERVICEDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "dockerserverman.h"
#include <QObject>

namespace Ui {
class AddDockerServiceDlg;
}

class WalletModel;
class SendCoinsRecipient;
class DockerCreateService;
class ResourceItem;

class CheckoutTransaction;
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
    DockerCreateService m_createService;
    CheckoutTransaction *m_checkoutTransaction;
    LoadingWin *m_loadingWin;
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
    CAmount getTxidAmount(std::string);
    
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
    void initTableWidget();
    void slot_buyClicked();

    void slot_refreshTransactionStatus();
    void transactionFinished();
    void refreshDNData();
    void slot_updateTaskTime(int);
    void slot_hireTimeChanged(int);

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
    void threadStoped();

private Q_SLOTS:
    void startCheckTransactiontTask();

};

#endif // ADDDOCKERSERVICEDLG_H
