#include "simplesendcoindlg.h"
#include "ui_simplesendcoindlg.h"
#include "init.h"
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "massgridunits.h"
#include "guiutil.h"
#include <math.h>
#include "sendcoinsdialog.h"
#include "askpassphrasedialog.h"
#include "massgridgui.h"
#include "amount.h"

extern CWallet* pwalletMain;
extern SendCoinsDialog* g_sendCoinsPage;

SimpleSendcoinDlg::SimpleSendcoinDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimpleSendcoinDlg)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pBtn_sendCoin, SIGNAL(clicked()), this, SLOT(onPBtn_sendCoinClicked()));

    ui->label_titlename->setText(tr("Service Detail"));
    this->setAttribute(Qt::WA_TranslucentBackground);
    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

SimpleSendcoinDlg::~SimpleSendcoinDlg()
{
    delete ui;
}

void SimpleSendcoinDlg::mousePressEvent(QMouseEvent* e)
{
    int posx = e->pos().x();
    int posy = e->pos().y();
    int framex = ui->mainframe->pos().x();
    int framey = ui->mainframe->pos().y();
    int frameendx = framex + ui->mainframe->width();
    int frameendy = framey + 30;
    if (posx > framex && posx < frameendx && posy > framey && posy < frameendy) {
        m_mousePress = true;
        m_last = e->globalPos();
    } else {
        m_mousePress = false;
    }
}

void SimpleSendcoinDlg::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_mousePress)
        return;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x() + dx, this->y() + dy));
}

void SimpleSendcoinDlg::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_mousePress)
        return;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x() + dx, this->y() + dy));
}

void SimpleSendcoinDlg::prepareOrderTransaction(WalletModel* model,const std::string& paytoAddress,const std::string& addr_port)
{
    m_walletModel = model;
    m_paytoAddress = paytoAddress;
    m_addr_port = addr_port;

    LogPrintf("---->prepareOrderTransaction m_paytoAddress:%s\n",m_paytoAddress);
    ui->payTo->setText(QString::fromStdString(m_paytoAddress));
}

std::string SimpleSendcoinDlg::getTxid()
{
    return m_txid;
}

void SimpleSendcoinDlg::onPBtn_sendCoinClicked()
{
    if(sendCoin()){
        QDialog::accept();
    }
}

bool SimpleSendcoinDlg::sendCoin()
{
    if(pwalletMain->IsLocked()){
        CMessageBox::information(this, tr("Wallet option"), tr("This option need to unlock your wallet"));

        if(!m_walletModel)
            return false;
        // Unlock wallet when requested by wallet model
        if (m_walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, 0);
            dlg.setModel(m_walletModel);
            QPoint pos = MassGridGUI::winPos();
            QSize size = MassGridGUI::winSize();
            dlg.move(pos.x()+(size.width()-dlg.width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-dlg.height()*GUIUtil::GetDPIValue())/2);

            if(dlg.exec() != QDialog::Accepted){
                return false;
            }
        }
    }

    SendCoinsRecipient recipient;

    if(!validate(recipient))
        return false;
    if (recipient.paymentRequest.IsInitialized())
        return false;

    // Normal payment
    recipient.address = ui->payTo->text();
    // recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();
    if(ui->checkUseInstantSend->isChecked())
        recipient.fUseInstantSend = true;
    else
    {
        recipient.fUseInstantSend = false;
    }
    
    // getValue(recipient);
    QList<SendCoinsRecipient> recipients;
    recipients.append(recipient);
    std::string txid = g_sendCoinsPage->send(recipients,"","",true,m_addr_port);

    if(!txid.size())
        return false;
    
    m_txid = txid;
    // m_amount = recipient.amount;

    return true;
}

bool SimpleSendcoinDlg::validate(SendCoinsRecipient& recipient)
{
    if (!m_walletModel)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    if (!m_walletModel->validateAddress(ui->payTo->text()))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if (!ui->payAmount->validate())
    {
        retval = false;
    }

    // Sending a zero amount is invalid
    if (ui->payAmount->value(0) <= 0)
    {
        ui->payAmount->setValid(false);
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(ui->payTo->text(), ui->payAmount->value())) {
        ui->payAmount->setValid(false);
        retval = false;
    }

    return retval;
}