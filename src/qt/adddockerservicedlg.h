#ifndef ADDDOCKERSERVICEDLG_H
#define ADDDOCKERSERVICEDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "dockerserverman.h"

namespace Ui {
class AddDockerServiceDlg;
}

class WalletModel;
class SendCoinsRecipient;
class DockerCreateService;
class ResourceItem;
class AddDockerServiceDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AddDockerServiceDlg(QWidget *parent = nullptr);
    ~AddDockerServiceDlg();

    void setaddr_port(const std::string& addr_poprt);

    void setWalletModel(WalletModel* walletmodel);

private:
    Ui::AddDockerServiceDlg *ui;

    QPoint m_last;
    bool m_mousePress;
    std::string m_addr_port;
    std::string m_txid;
    int m_amount;
    std::string m_masterndoeAddr;
    WalletModel *m_walletModel;
    DockerCreateService m_createService;

private:
    bool createDockerService();

    void doStep1(ResourceItem*);

    bool sendCoin();
    bool validate(SendCoinsRecipient&);
    void askForDNData();
    void loadResourceData();
    bool isTransactionFinished(std::string& strErr);
    void checkCreateTaskStatus(std::string txid);
    bool deleteService(const std::string& strServiceid);
    void gotoStep1Page();
    QString getErrorMsg(int errCode);
    
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
    void slot_timeOut();

    void refreshServerList();
    void initTableWidget();
    void slot_buyClicked();

    void slot_refreshTransactionStatus();
    void refreshDNData();
};

#endif // ADDDOCKERSERVICEDLG_H
