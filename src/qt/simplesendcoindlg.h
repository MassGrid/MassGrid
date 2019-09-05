#ifndef SIMPLESENDCOINDLG_H
#define SIMPLESENDCOINDLG_H

#include <QDialog>
#include <QMouseEvent>
#include "amount.h"
#include "networkers.h"

class WalletModel;
class SendCoinsRecipient;
class AskDNDataWorker;

namespace Ui {
class SimpleSendcoinDlg;
}

class SimpleSendcoinDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleSendcoinDlg(QWidget *parent = 0);
    ~SimpleSendcoinDlg();
    void prepareOrderTransaction(WalletModel* model,const std::string& serviceID,const std::string& paytoAddress,const std::string& addr_port,CAmount machinePrice);
    std::string getTxid();

private:
    Ui::SimpleSendcoinDlg *ui;
    WalletModel* m_walletModel;
    AskDNDataWorker *m_askDNDataWorker;

    QPoint m_last;
    bool m_mousePress;

    std::string m_txid;
    CAmount m_amount;
    std::string m_paytoAddress;
    std::string m_addr_port;
    std::string m_serviceID;

private:
    bool sendCoin();
    bool validate(SendCoinsRecipient& recipient);
    void askForDNData();
    void startAskDNDataWork(const char* slotMethod,bool needAsk);
    void saveMachinePrice();
    CAmount getMachinePrice();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

public Q_SLOTS:
    void onPBtn_sendCoinClicked();
    void onHireTimeChanged(int value);
    void updateServiceListFinished(bool isTaskFinished);
    void updateCreateServerWaitTimer(int);
};

#endif // SIMPLESENDCOINDLG_H
