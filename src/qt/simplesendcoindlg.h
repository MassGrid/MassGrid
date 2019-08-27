#ifndef SIMPLESENDCOINDLG_H
#define SIMPLESENDCOINDLG_H

#include <QDialog>
#include <QMouseEvent>

class WalletModel;
class SendCoinsRecipient;

namespace Ui {
class SimpleSendcoinDlg;
}

class SimpleSendcoinDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleSendcoinDlg(QWidget *parent = 0);
    ~SimpleSendcoinDlg();
    // void setWalletModel(WalletModel* model);
    // void setPaytoAddress(const std::string& address);
    void prepareOrderTransaction(WalletModel* model,const std::string& paytoAddress,const std::string& addr_port);
    std::string getTxid();

private:
    Ui::SimpleSendcoinDlg *ui;
    WalletModel* m_walletModel;

    QPoint m_last;
    bool m_mousePress;

    std::string m_txid;
    // CAmount m_amount;
    std::string m_paytoAddress;
    std::string m_addr_port;

private:
    bool sendCoin();
    bool validate(SendCoinsRecipient& recipient);

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

public Q_SLOTS:
    void onPBtn_sendCoinClicked();
};

#endif // SIMPLESENDCOINDLG_H
