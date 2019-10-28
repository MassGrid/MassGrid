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
#include "dockercluster.h"
#include "adddockerservicedlg.h"
#include "loadingwin.h"
#include <QTimer>
#include "validation.h"
#include <QRegExp>

extern CWallet* pwalletMain;
extern SendCoinsDialog* g_sendCoinsPage;

SimpleSendcoinDlg::SimpleSendcoinDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimpleSendcoinDlg),
    m_askDNDataWorker(NULL),
    m_checkoutTransaction(NULL)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pBtn_sendCoin, SIGNAL(clicked()), this, SLOT(onPBtn_sendCoinClicked()));
    connect(ui->horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(onHireTimeChanged(int)));

    ui->label_titlename->setText(tr("Service Detail"));
    this->setAttribute(Qt::WA_TranslucentBackground);
    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

SimpleSendcoinDlg::~SimpleSendcoinDlg()
{
    delete ui;
    LoadingWin::hideLoadingWin();
    stopAndDelTransactionThread();
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

void SimpleSendcoinDlg::prepareOrderTransaction(WalletModel* model,const std::string& serviceID,const std::string& paytoAddress,const std::string& addr_port,CAmount machinePrice)
{
    m_walletModel = model;
    m_paytoAddress = paytoAddress;
    m_addr_port = addr_port;
    m_amount = machinePrice;
    m_serviceID = serviceID;

    ui->payTo->setText(QString::fromStdString(m_paytoAddress));
    ui->payAmount->setFocus();

    askForDNData();
}

bool SimpleSendcoinDlg::reconnectDockerNetwork()
{
    QRegExp rx("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");
    if(QString::fromStdString(m_addr_port).contains(":") && !rx.exactMatch(QString::fromStdString(m_addr_port).split(":").at(0)))
    {
        LogPrintf("m_addr_port:%s is failed\n",m_addr_port);
        return false;
    }

    dockercluster.ProcessDockernodeDisconnections(m_addr_port);
    QThread::msleep(100);
    if(!dockercluster.SetConnectDockerAddress(m_addr_port) || !dockercluster.ProcessDockernodeConnections()){
        CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!") + "<br>" + QString("masternode ip_port:") + QString::fromStdString(m_addr_port));
        LogPrintf("MasternodeList deleteService failed\n");
        return false;
    }
}

void SimpleSendcoinDlg::startAskDNDataWork(const char* slotMethod,bool needAsk)
{
    LogPrintf("start to ask dndata work:%s\n",m_addr_port);

    // if(!dockercluster.SetConnectDockerAddress(m_addr_port) || !dockercluster.ProcessDockernodeConnections()){
    if(!reconnectDockerNetwork()){
        CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!"));
        LogPrintf("Connect docker network failed!m_addr_port is:%s\n",m_addr_port);
        LoadingWin::hideLoadingWin();
        close();        
    }

    if(m_askDNDataWorker == NULL){
        m_askDNDataWorker = new AskDNDataWorker(0);
        QThread *workThread = new QThread(0);
        m_askDNDataWorker->moveToThread(workThread);
        connect(workThread,SIGNAL(started()),m_askDNDataWorker,SLOT(startTask()));
        connect(m_askDNDataWorker,SIGNAL(updateTaskTime(int)),this,SLOT(updateCreateServerWaitTimer(int)));
        connect(m_askDNDataWorker,SIGNAL(askDNDataFinished(bool)),workThread,SLOT(quit()));
    }

    connect(m_askDNDataWorker,SIGNAL(askDNDataFinished(bool)),this,slotMethod);

    if(needAsk)
        dockercluster.AskForDNData();
    
    m_askDNDataWorker->thread()->start();
}

void SimpleSendcoinDlg::updateCreateServerWaitTimer(int)
{

}

CAmount SimpleSendcoinDlg::getMachinePrice()
{
    ServiceInfo serviceInfo = dockercluster.vecServiceInfo.servicesInfo[m_serviceID];
    std::map<Item, Value_price> items = dockercluster.machines.items; 

    Item machineItem(serviceInfo.CreateSpec.hardware.CPUType,
                    serviceInfo.CreateSpec.hardware.CPUThread,
                    serviceInfo.CreateSpec.hardware.MemoryType,
                    serviceInfo.CreateSpec.hardware.MemoryCount,
                    serviceInfo.CreateSpec.hardware.GPUType,
                    serviceInfo.CreateSpec.hardware.GPUCount);
    Value_price itemPrice = items[machineItem];

    if(itemPrice.price > 0)
        return itemPrice.price;
    else
        return m_amount;
}

void SimpleSendcoinDlg::askForDNData()
{
    LoadingWin::showLoading2(tr("Load docer resource..."));

    const char* method = SLOT(updateServiceListFinished(bool));
    startAskDNDataWork(method,true);
}

void SimpleSendcoinDlg::updateServiceListFinished(bool isTaskFinished)
{
    disconnect(m_askDNDataWorker,SIGNAL(askDNDataFinished(bool)),this,SLOT(updateServiceListFinished(bool)));
    LoadingWin::hideLoadingWin();

    if(isTaskFinished){
        CAmount price = getMachinePrice();
        ui->payAmount->setValue(price);

        LogPrintf("AddDockerServiceDlg get DNData Status:CDockerServerman::Received\n");
    }
    else
    {
        //time out tip
        CMessageBox::information(this, tr("Load failed"),tr("Can't load Docker configuration!"));
        close();
        return;
    }
}

std::string SimpleSendcoinDlg::getTxid()
{
    return m_txid;
}

void SimpleSendcoinDlg::onPBtn_sendCoinClicked()
{
    if(sendCoin()){
        LoadingWin::showLoading2(tr("Chcke transaction......"));
        //todo checkout transaction
        startCheckTransactionWork();
    }
}

void SimpleSendcoinDlg::saveMachinePrice()
{
    LOCK2(cs_main, pwalletMain->cs_wallet);
    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(getTxid())];
    wtx.Setprice(std::to_string(getMachinePrice()));
    CWalletDB walletdb(pwalletMain->strWalletFile);
    wtx.WriteToDisk(&walletdb);
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
    recipient.amount = ui->payAmount->value();
    if(ui->checkUseInstantSend->isChecked())
        recipient.fUseInstantSend = true;
    else
    {
        recipient.fUseInstantSend = false;
    }
    
    QList<SendCoinsRecipient> recipients;
    recipients.append(recipient);
    std::string txid = g_sendCoinsPage->send(recipients,"","",true,m_addr_port);

    if(!txid.size())
        return false;
    
    m_txid = txid;

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

void SimpleSendcoinDlg::onHireTimeChanged(int value)
{
    ui->label_hireTime->setText(QString("(%1H)").arg(QString::number(value)));
    ui->payAmount->setValue(m_amount*value);
}

void SimpleSendcoinDlg::startCheckTransactionWork()
{
    if(m_checkoutTransaction == NULL){
        m_checkoutTransaction = new CheckoutTransaction(m_txid,0);
        QThread *workThread = new QThread();
        m_checkoutTransaction->moveToThread(workThread);
        connect(workThread,SIGNAL(started()),m_checkoutTransaction,SLOT(startTask()));
        connect(m_checkoutTransaction,SIGNAL(checkTransactionFinished()),this,SLOT(transactionFinished()));
        connect(m_checkoutTransaction,SIGNAL(updateTaskTime(int)),this,SLOT(slot_updateTaskTime(int)));
        connect(m_checkoutTransaction,SIGNAL(checkTransactionFinished()),workThread,SLOT(quit()));
    }

    m_checkoutTransaction->thread()->start();
}

void SimpleSendcoinDlg::stopAndDelTransactionThread()
{
    if(m_checkoutTransaction != NULL){
        disconnect(m_checkoutTransaction,SIGNAL(checkTransactionFinished()),this,SLOT(transactionFinished()));
        disconnect(m_checkoutTransaction,SIGNAL(updateTaskTime(int)),this,SLOT(slot_updateTaskTime(int)));

        QEventLoop loop;
        connect(m_checkoutTransaction,SIGNAL(threadStopped()),&loop,SLOT(quit()));

        m_checkoutTransaction->setNeedToWork(false);
        loop.exec();
        delete m_checkoutTransaction; 
        m_checkoutTransaction == NULL;    
    }
}

void SimpleSendcoinDlg::slot_updateTaskTime(int index)
{
    ui->label_refreshPayStatus->setText(QString::number(index));
}

void SimpleSendcoinDlg::transactionFinished()
{
    LoadingWin::hideLoadingWin();
    doTaskAfterFinishedSendcoins();
}

void SimpleSendcoinDlg::doTaskAfterFinishedSendcoins()
{
    saveMachinePrice();
    QDialog::accept();
}
