#include "adddockerservicedlg.h"
#include "ui_adddockerservicedlg.h"
#include "../dockercluster.h"
#include "../rpc/protocol.h"
#include "cmessagebox.h"
#include "init.h"
#include "wallet/wallet.h"
#include "../walletview.h"
#include "walletmodel.h"
#include "askpassphrasedialog.h"
#include "massgridgui.h"
#include <QFileDialog>
#include <QFile>
#include <QTimer>
#include "askpassphrasedialog.h"
#include "coincontroldialog.h"
#include "coincontrol.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "sendcoinsdialog.h"
#include "resourceitem.h"
#include "instantx.h"
#include "dockerserverman.h"
#include "loadingwin.h"
#include <QThread>
#include <QStringList>
#include <QEventLoop>
#include <QStyleFactory>
#include "cupdatethread.h"
#include "guiutil.h"

#define LOADRESOURCETIMEOUT 30

extern SendCoinsDialog* g_sendCoinsPage;

QStringList ServiceManCodeStr = {
    QString("Create sucess"),
    QString("Sigtime error"),
    QString("Docker network version error"),
    QString("Check sigtime error"),
    QString("No transaction"),
    QString("Transaction no confirms"),
    QString("Transaction double create"),
    QString("Service item no found"),
    QString("Service item no resource"),
    QString("Payment not enough"),
    QString("GPU amount error"),
    QString("CPU amount error"),
    QString("Member amount error"),
    QString("Transaction double tlement"),
    QString("Pubkey error")
};

AddDockerServiceDlg::AddDockerServiceDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDockerServiceDlg),
    m_walletModel(NULL),
    m_checkoutTransaction(NULL),
    m_askDNDataWorker(NULL),
    m_loadingWin(NULL)
{
    ui->setupUi(this);

    ui->label_titleName->setText(tr("Create Service"));
    this->setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);

 #ifdef Q_OS_MAC
    ui->comboBox_image->setStyle(QStyleFactory::create("Windows"));
    ui->comboBox_gpuType->setStyle(QStyleFactory::create("Windows"));
#endif

    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(slot_close()));
    // connect(ui->openPubKeyButton,SIGNAL(clicked()),this,SLOT(slot_openPubKeyFile()));
    connect(ui->lineEdit_sshpubkey,SIGNAL(returnPressed()),this,SLOT(slot_openPubKeyFile()));
    
    connect(ui->horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(slot_hireTimeChanged(int)));
    // connect(ui->minAmountButton,SIGNAL(clicked()),this,SLOT(slot_searchMinAmount()));
    connect(ui->lineEdit_minAmount,SIGNAL(textChanged(QString)),this,SLOT(slot_minAmounttextChanged(QString)));
    connect(ui->lineEdit_minAmount,SIGNAL(returnPressed()),this,SLOT(slot_searchMinAmount()));

    connect(ui->nextButton_2,SIGNAL(clicked()),this,SLOT(doTransaction()));
    connect(ui->nextButton_3,SIGNAL(clicked()),this,SLOT(doStep3()));
    connect(ui->nextButton_4,SIGNAL(clicked()),this,SLOT(doStep4()));
    connect(ui->comboBox_gpuType,SIGNAL(currentIndexChanged(int)),this,SLOT(slot_gpuComboxCurrentIndexChanged(int)));

    ui->stackedWidget->setCurrentIndex(0);

    ui->tableWidget_resource->verticalHeader()->setVisible(false);
    ui->tableWidget_resource->horizontalHeader()->setVisible(false); 

    QTimer::singleShot(1000,this,SLOT(initTableWidget()));

    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

AddDockerServiceDlg::~AddDockerServiceDlg()
{
    delete ui;
    stopAndDelTransactionThread();
    if(m_loadingWin != NULL){
        m_loadingWin->hideWin();
        delete m_loadingWin;
    }
}

void AddDockerServiceDlg::mousePressEvent(QMouseEvent *e)
{
    int posx = e->pos().x();
    int posy = e->pos().y();
    int framex = ui->mainframe->pos().x();
    int framey = ui->mainframe->pos().y();
    int frameendx = framex+ui->mainframe->width();
    int frameendy = framey+30*GUIUtil::GetDPIValue();
    if(posx>framex && posx<frameendx && posy>framey && posy<frameendy){
        m_mousePress = true;
        m_last = e->globalPos();
    }
    else{
        m_mousePress = false;
    }
}

void AddDockerServiceDlg::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void AddDockerServiceDlg::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void AddDockerServiceDlg::slot_close()
{
    if(ui->stackedWidget->currentIndex() == 1){
        if(m_txid.size()){
            CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Quit"),
                tr("After exiting, you can query the transfer result in the order list and proceed to the next step,are you sure close it？"),
                CMessageBox::Ok_Cancel, CMessageBox::Cancel);

            if(btnRetVal == CMessageBox::Cancel)
                return;
        }
    }
    else if(ui->stackedWidget->currentIndex() == 2){
        CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Quit"),
                tr("The transfer has been completed and the service is being created. If you exit now, you will receive a refund within 15-60 minutes,are sure close it?"),
                CMessageBox::Ok_Cancel, CMessageBox::Cancel);

        if(btnRetVal == CMessageBox::Cancel){
            close();
        }
        deleteService(m_txid);
    }
    // else if(ui->stackedWidget->currentIndex() == 3){
    //     CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("退出"),
    //     tr("服务创建中，？"),
    //     CMessageBox::Ok_Cancel, CMessageBox::Cancel);

    //     if(btnRetVal == CMessageBox::Cancel)
    //         return;
    // }
    close();
}

bool AddDockerServiceDlg::deleteService(const std::string& strServiceid)
{
    if(!dockercluster.SetConnectDockerAddress(m_addr_port) || !dockercluster.ProcessDockernodeConnections()){
        CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!"));
        return false;
    }

    DockerDeleteService delService{};

    delService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
    delService.txid = dockercluster.dndata.mapDockerServiceLists[strServiceid].txid;

    if(!dockercluster.DeleteAndSendServiceSpec(delService)){
        CMessageBox::information(this, tr("Docker option"),tr("Delete docker service failed!"));
    }
    return true;
}

void AddDockerServiceDlg::slot_nextStep()
{
    int curPageIndex = ui->stackedWidget->currentIndex();

    switch (curPageIndex)
    {
        case 0 :
            ui->label_step2->setEnabled(true);
            ui->line_2->setEnabled(true);
            ui->label_14->setEnabled(true);
            break;
        case 1 : 
            ui->line_3->setEnabled(true);
            ui->label_step3->setEnabled(true);
            ui->label_15->setEnabled(true);
            break;
        case 2 :
            ui->label_step4->setEnabled(true);
            ui->label_17->setEnabled(true);
            break;
        case 3 :
            close();
            return;    
        default:
            break;
    }
    ui->stackedWidget->setCurrentIndex(curPageIndex+1);

}

void AddDockerServiceDlg::doStep2()
{
    showLoading(tr("Check transaction..."));
    ui->nextButton_2->setEnabled(false);

    //start to check transacation
    startCheckTransactionWork();
}

void AddDockerServiceDlg::startCheckTransactionWork()
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

void AddDockerServiceDlg::stopAndDelTransactionThread()
{
    if(m_checkoutTransaction != NULL){
        disconnect(m_checkoutTransaction,SIGNAL(checkTransactionFinished()),this,SLOT(transactionFinished()));
        disconnect(m_checkoutTransaction,SIGNAL(updateTaskTime(int)),this,SLOT(slot_updateTaskTime(int)));

        QEventLoop loop;
        connect(m_checkoutTransaction,SIGNAL(threadStopped()),&loop,SLOT(quit()));

        m_checkoutTransaction->setNeedToWork(false);
        loop.exec();
        delete m_checkoutTransaction;        
    }
}

void AddDockerServiceDlg::doTransaction()
{
    if(!sendCoin())
        return ;

    doStep2();
}

void AddDockerServiceDlg::slot_updateTaskTime(int index)
{
    ui->label_refreshPayStatus->setText(QString::number(index));
    ui->label_CreatePageTimer->setText(QString::number(index));
}

void AddDockerServiceDlg::updateCreateServerWaitTimer(int index)
{
    ui->label_CreatePageTimer->setText(QString::number(index));
}

void AddDockerServiceDlg::slot_refreshTransactionStatus()
{
    static int index = 0;
    ui->label_refreshPayStatus->setText(QString::number(++index));
    QTimer::singleShot(1000,this,SLOT(slot_refreshTransactionStatus()));
}

void AddDockerServiceDlg::transactionFinished()
{
    hideLoadingWin();
    slot_nextStep();
    doStep3();
}

void AddDockerServiceDlg::doStep3()
{
    ui->nextButton_3->setEnabled(false);
    showLoading(tr("Create Service..."));
    if(createDockerService()){
        //do not need to ask dndata
        QThread::sleep(1);
        startAskDNDataWork(SLOT(updateDNDataAfterCreate(bool)),false);
    }
    else
    {
        QString msg = tr("Transaction has been finished,") + tr("create service failed") + tr(",is need to re-select resource or go into the order list page to apply for a refund?");
        CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Create Failed"),
            msg,CMessageBox::Ok_Cancel, CMessageBox::Cancel);

        if(btnRetVal == CMessageBox::Cancel){
            close();
        }
        
        ui->nextButton_3->setText(tr("re-Create"));
        ui->nextButton_3->setEnabled(true);
    }
}

void AddDockerServiceDlg::doStep4()
{
    accept();
}

void AddDockerServiceDlg::slot_openPubKeyFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("open Pubkey file"),"",tr("Pubkey File (*.pub)"));

    if(fileName.isEmpty()){
        return ;
    }

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        CMessageBox::information(this, tr("Error"), tr("open Pubkey file error!"));
        return ;
    }
    ui->lineEdit_sshpubkey->setText(file.readAll());
}

void AddDockerServiceDlg::slot_searchMinAmount()
{
    expansionResourceItems();
    QString minAmount = ui->lineEdit_minAmount->text();
    if(minAmount.isEmpty())
        return ;
    
    double payment = ui->lineEdit_minAmount->text().toDouble();
    int rowCount = ui->tableWidget_resource->rowCount();
    
    for(int i=0;i<rowCount;i++){
        QString itemText = ui->tableWidget_resource->item(i,1)->text();
        QString amountStr = MassGridUnits::formatWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(),itemText.toDouble());
        double amount = QString(amountStr.split(" ").at(0)).toDouble();

        if(payment - amount < 0){
            ui->tableWidget_resource->hideRow(i);
        }
    }
}

void AddDockerServiceDlg::slot_minAmounttextChanged(QString text)
{
    if(!text.isEmpty())
        return ;
    
    expansionResourceItems();
}

void AddDockerServiceDlg::setaddr_port(const std::string& addr_port)
{
    m_addr_port = addr_port; 
    LogPrintf("AddDockerServiceDlg::setaddr_port m_addr_port:%s\n",addr_port);
}

void AddDockerServiceDlg::settxid(const std::string& txid)
{
    m_txid = txid;
}

void AddDockerServiceDlg::setWalletModel(WalletModel* walletmodel)
{
    m_walletModel = walletmodel;
}

bool AddDockerServiceDlg::createDockerService()
{
    if(!m_addr_port.size()){
        LogPrintf("connect docker address error!\n");
        return false ;
    }

    std::string strssh_pubkey = ui->lineEdit_sshpubkey->text().toStdString().c_str();
    m_createService.ssh_pubkey = strssh_pubkey;

    m_createService.fPersistentStore = ui->checkBox_persistentStore->isChecked();

    if(!strssh_pubkey.size()){
        CMessageBox::information(this, tr("Docker option"), tr("SSH public key is empty!"));
        return false;
    }

    bool lockedFlag = pwalletMain->IsLocked();

    if(lockedFlag){
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

    if(!dockercluster.SetConnectDockerAddress(m_addr_port)){
        LogPrintf("get invalid IP!\n");
        return false;
    }
    if(!dockercluster.ProcessDockernodeConnections()){
        LogPrintf("Connect to Masternode failed!\n");
         return false;
    }

    m_createService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
    m_createService.txid = uint256S(m_txid);

    std::string strServiceName = ui->lineEdit_name->text().toStdString().c_str();
    m_createService.serviceName = strServiceName;

    std::string strServiceImage = QString("massgrid/" + ui->comboBox_image->currentText()).toStdString().c_str();

    m_createService.image = strServiceImage;

    // std::string strn2n_Community = ui->lineEdit_n2n_name->text().toStdString().c_str();
    m_createService.n2n_community = "massgridn2n"; //strn2n_Community;
    if(!dockercluster.CreateAndSendSeriveSpec(m_createService)){
        LogPrintf("dockercluster.CreateAndSendSeriveSpec error\n");
        return false;
    }

    //lock wallet
    if(m_walletModel)
        m_walletModel->setWalletLocked(true);

    return true;
}

void AddDockerServiceDlg::updateDNDataAfterCreate(bool isFinished)
{
    disconnect(m_askDNDataWorker,SIGNAL(askDNDataFinished(bool)),this,SLOT(updateDNDataAfterCreate(bool)));
    hideLoadingWin();
    if(!isFinished){
        //do something when timeout
        CMessageBox::information(this, tr("Load failed"),tr("Create service have't respone,please check you netwowrk is running!"));
        return ;
    }
    std::map<std::string,Service> serverlist = dockercluster.dndata.mapDockerServiceLists;
    std::map<std::string,Service>::iterator iter = serverlist.begin();
    for(;iter != serverlist.end();iter++){
        QString id = QString::fromStdString(iter->first);
        Service service = serverlist[iter->first];
        if(m_txid == service.txid.ToString()){
            slot_nextStep();
            return ;
        }
    }

    if(dockercluster.dndata.errCode != 0){
        QString errStr = ServiceManCodeStr[dockercluster.dndata.errCode];

        switch (dockercluster.dndata.errCode)
        {
            case SERVICEMANCODE::SIGTIME_ERROR:
            case SERVICEMANCODE::VERSION_ERROR:
            case SERVICEMANCODE::CHECKSIGNATURE_ERROR:
            case SERVICEMANCODE::NO_THRANSACTION:
            case SERVICEMANCODE::TRANSACTION_NOT_CONFIRMS:
            case SERVICEMANCODE::TRANSACTION_DOUBLE_CREATE:
            case SERVICEMANCODE::TRANSACTION_DOUBLE_TLEMENT:{
                QString msg = tr("Transaction error:") + errStr +tr(",the window will be close!");
                CMessageBox::information(this, tr("Create Failed"),msg);
                close();
                return ;
            }
            case SERVICEMANCODE::SERVICEITEM_NOT_FOUND:
            case SERVICEMANCODE::SERVICEITEM_NO_RESOURCE:
            case SERVICEMANCODE::PAYMENT_NOT_ENOUGH:
            case SERVICEMANCODE::GPU_AMOUNT_ERROR:
            case SERVICEMANCODE::CPU_AMOUNT_ERROR:
            case SERVICEMANCODE::MEM_AMOUNT_ERROR:
            case SERVICEMANCODE::PUBKEY_ERROR:{
                QString msg = tr("Transaction has been finished,") + errStr + tr(",is need to re-select resource or go into the order list page to apply for a refund?");
                CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Create Failed"),
                    msg,CMessageBox::Ok_Cancel, CMessageBox::Cancel);

                if(btnRetVal == CMessageBox::Cancel){
                    close();
                    return ;
                }
                else{
                    gotoStep1Page();
                }
            }
            default:
                break;
        }
    }
}

QString AddDockerServiceDlg::getErrorMsg(int errCode)
{
    switch (errCode)
    {
        case SERVICEMANCODE::SIGTIME_ERROR:
            return tr("Transaction Sigtime failed!");
        case SERVICEMANCODE::VERSION_ERROR:
            return tr("Docker version failed!");
        case SERVICEMANCODE::SERVICEITEM_NOT_FOUND:
            return tr("Can't find service item!");  
        case SERVICEMANCODE::GPU_AMOUNT_ERROR:
        case SERVICEMANCODE::CPU_AMOUNT_ERROR:
        case SERVICEMANCODE::MEM_AMOUNT_ERROR:
            return tr("Have't enough resource can be find!");    
        default:
            return tr("Create failed!");
            break;
    }
}

void AddDockerServiceDlg::gotoStep1Page()
{
    ui->label_step2->setEnabled(false);
    ui->line_2->setEnabled(false);
    ui->label_14->setEnabled(false);

    ui->line_3->setEnabled(false);
    ui->label_step3->setEnabled(false);
    ui->label_15->setEnabled(false);

    ui->label_step4->setEnabled(false);
    ui->label_17->setEnabled(false);

    ui->nextButton_2->setEnabled(true);
    ui->nextButton_3->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(0);

    askForDNData();
}

void AddDockerServiceDlg::checkCreateTaskStatus(std::string txid)
{

}


bool AddDockerServiceDlg::sendCoin()
{

    bool lockedFlag = pwalletMain->IsLocked();

    if(lockedFlag){
        CMessageBox::information(this, tr("Wallet option"), tr("This option need to unlock your wallet"));

        if(!m_walletModel)
            return false;
        // Unlock wallet when requested by wallet model
        if (m_walletModel->getEncryptionStatus() == WalletModel::Locked)
        // if(m_walletModel->isLocked())
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
    m_amount = recipient.amount;

    return true;
}

bool AddDockerServiceDlg::validate(SendCoinsRecipient& recipient)
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

void AddDockerServiceDlg::showLoading(const QString & msg)
{
    if(m_loadingWin == NULL)
        m_loadingWin = new LoadingWin(ui->centerWin);

    QPoint pos = ui->centerWin->pos(); //MassGridGUI::winPos();
    QSize size = ui->centerWin->size(); //MassGridGUI::winSize();
    m_loadingWin->move(pos.x()+(size.width()-m_loadingWin->width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-m_loadingWin->height()*GUIUtil::GetDPIValue())/2);
    
    m_loadingWin->showLoading(msg);
}

void AddDockerServiceDlg::hideLoadingWin()
{
    if(m_loadingWin != NULL){
        m_loadingWin->hideWin();
        delete m_loadingWin;
        m_loadingWin = NULL;
    }
}

void AddDockerServiceDlg::askForDNData()
{
    showLoading(tr("Load docer resource..."));
    // dockercluster.AskForDNData();
    // refreshServerList();
    const char* method = SLOT(updateServiceListFinished(bool));
    startAskDNDataWork(method,true);
}

void AddDockerServiceDlg::startAskDNDataWork(const char* slotMethod,bool needAsk)
{
    if(!dockercluster.SetConnectDockerAddress(m_addr_port) || !dockercluster.ProcessDockernodeConnections()){
        CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!"));
        return ;
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

void AddDockerServiceDlg::updateServiceListFinished(bool isTaskFinished)
{
    disconnect(m_askDNDataWorker,SIGNAL(askDNDataFinished(bool)),this,SLOT(updateServiceListFinished(bool)));
    hideLoadingWin(); 

    if(isTaskFinished){
        loadResourceData();
        LogPrintf("AddDockerServiceDlg get DNData Status:CDockerServerman::Received\n");
    }
    else
    {
        //time out tip
        CMessageBox::information(this, tr("Load failed"),tr("Can't load Docker configuration!"));
        return;
    }
}

void AddDockerServiceDlg::refreshServerList()
{
    loadResourceData();
    LogPrintf("AddDockerServiceDlg get DNData Status:CDockerServerman::Received\n");
    hideLoadingWin();
}

void AddDockerServiceDlg::filterResource(std::string txid)
{
    CAmount payment = GUIUtil::getTxidAmount(txid)*(-1);
    int rowCount = ui->tableWidget_resource->rowCount();

    for(int i=0;i<rowCount;i++){
        // ResourceItem* item = qobject_cast<ResourceItem *>(ui->tableWidget_resource->setCellWidget(i,0));
        QString amount = ui->tableWidget_resource->item(i,1)->text();
        if(payment - amount.toDouble() < 0){
            ui->tableWidget_resource->hideRow(i);
        }
    }
}

void AddDockerServiceDlg::initTableWidget()
{
    ui->tableWidget_resource->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_resource->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_resource->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_resource->setAlternatingRowColors(true);

    int itemwidth = ui->tableWidget_resource->width()/8;
    ui->tableWidget_resource->setColumnWidth(0,ui->tableWidget_resource->width());

    askForDNData();
}

void AddDockerServiceDlg::loadResourceData()
{
    QStringList images;
    CUpdateThread::getImagesData(images);

    if(images.size()){
        ui->comboBox_image->clear();
    }

    ui->comboBox_image->addItems(images);

    std::map<Item,Value_price> items = dockercluster.dndata.items;
    m_masterndoeAddr = dockercluster.dndata.masternodeAddress;
    std::map<Item,Value_price>::iterator iter = items.begin();

    ui->tableWidget_resource->setRowCount(0);
    ui->tableWidget_resource->setColumnCount(3);
    ui->tableWidget_resource->hideColumn(1);
    ui->tableWidget_resource->hideColumn(2);

    initCombobox();

    if(dockercluster.dndata.fPersistentStore){
        ui->checkBox_persistentStore->setEnabled(true);
        ui->checkBox_persistentStore->setChecked(true);
        ui->checkBox_persistentStore->setToolTip(tr("This node support persistable storage!"));
    }
    else{
        ui->checkBox_persistentStore->setToolTip(tr("This node does not support persistable storage!"));
    }

    int count = 0;

    for(;iter != items.end();iter++){

        ui->tableWidget_resource->setRowCount(count+1);
        ResourceItem * item = new ResourceItem(ui->tableWidget_resource);
        QString amount = MassGridUnits::formatHtmlWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), iter->second.price) + "/H";

        item->loadResourceData(QString::fromStdString(iter->first.gpu.Name),QString::number(iter->first.gpu.Count),
                               QString::fromStdString(iter->first.mem.Name),QString::number(iter->first.mem.Count),
                               QString::fromStdString(iter->first.cpu.Name),QString::number(iter->first.cpu.Count),
                               QString::number(iter->second.count),amount);
        item->setAmount(iter->second.price);
        connect(item,SIGNAL(sig_buyClicked()),this,SLOT(slot_buyClicked()));

        item->resize(ui->tableWidget_resource->width(),item->height()*GUIUtil::GetDPIValue());
        ui->tableWidget_resource->setRowHeight(count,item->height());
        ui->tableWidget_resource->setCellWidget(count,0,item);

        ui->tableWidget_resource->setItem(count,1,new QTableWidgetItem(QString::number(iter->second.price)));
        ui->tableWidget_resource->setItem(count,2,new QTableWidgetItem(QString::fromStdString(iter->first.gpu.Name)));

        int comboxCount = ui->comboBox_gpuType->count();
        QString GPUName = QString::fromStdString(iter->first.gpu.Name);
        bool isContains = false;

        for(int i=0;i<comboxCount;i++){
            if(GPUName.contains(ui->comboBox_gpuType->itemText(i))){
                // ui->comboBox_gpuType->addItem(GPUName);
                isContains = true;
                break;
            }
        }
        //add gpu name item to combobox
        if(!isContains)
            ui->comboBox_gpuType->addItem(GPUName);

        count++;
    }

    if(m_txid.size())
        filterResource(m_txid);
}

void AddDockerServiceDlg::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void AddDockerServiceDlg::slot_buyClicked()
{
    QString errStr;
    if(!ui->lineEdit_name->text().size()){
        errStr = tr("Service name is Empty!");
    }
    if(!ui->lineEdit_sshpubkey->text().size()){
        errStr = tr("Ssh pubkey is Empty!");
    }

    if(errStr.size()){
        CMessageBox::information(this, tr("Create Error"),errStr);
        return ;
    }

    ResourceItem* item = dynamic_cast<ResourceItem *>(QObject::sender());
    doStep1(item);

    if(!m_txid.size()){
        return ;
    }

    doStep2();

    // std::string strErr;
    // if(!CheckoutTransaction::isTransactionFinished(m_txid,strErr)){
    //     CMessageBox::information(this, tr("Transaction Error"),QString::fromStdString(strErr));
    //     return ;
    // }
    // slot_nextStep();
    // doStep3();
}

void AddDockerServiceDlg::doStep1(ResourceItem* item)
{
    int index = 0;
    QString gpuName = item->getGPUName();
    QString gpuCount = item->getGPUCount();
    QString romType = item->getRomType();
    QString romCount = item->getRomCount();
    QString cpuName = item->getCPUName();
    QString cpuCount = item->getCPUCount();
    QString availibleCount = item->getAvailibleCount();
    CAmount amount = item->getAmount();

    m_createService.item.cpu.Count = cpuCount.toInt();
    m_createService.item.cpu.Name = cpuName.toStdString().c_str();

    m_createService.item.mem.Count = romCount.toInt();
    m_createService.item.mem.Name = romType.toStdString().c_str();

    m_createService.item.gpu.Name = gpuName.toStdString().c_str();
    m_createService.item.gpu.Count = gpuCount.toInt();

    ui->payAmount->setValue(amount);
    ui->payTo->setText(QString::fromStdString(m_masterndoeAddr));
    ui->payAmount->setFocus();
    m_amount = amount;
    slot_nextStep();
}

void AddDockerServiceDlg::slot_hireTimeChanged(int value)
{
    ui->label_hireTime->setText(QString("(%1H)").arg(QString::number(value)));
    ui->payAmount->setValue(m_amount*value);
}

void AddDockerServiceDlg::initCombobox()
{
    ui->comboBox_gpuType->clear();
    ui->comboBox_gpuType->addItem(tr("Select GPU type"));
}

void AddDockerServiceDlg::expansionResourceItems()
{
    int rowCount = ui->tableWidget_resource->rowCount();

    for(int i=0;i<rowCount;i++){
        ui->tableWidget_resource->showRow(i);
    }

}

void AddDockerServiceDlg::slot_gpuComboxCurrentIndexChanged(int index)
{
    expansionResourceItems();
    int rowCount = ui->tableWidget_resource->rowCount();

    if(index < 1)
        return ;

    QString curText = ui->comboBox_gpuType->currentText();
    
    for(int i=0;i<rowCount;i++){
        QString gpuType = ui->tableWidget_resource->item(i,2)->text();
        if(!curText.contains(gpuType)){
            ui->tableWidget_resource->hideRow(i);
        }
    }
}

CheckoutTransaction::CheckoutTransaction(std::string txid,QObject* parent) :
    QObject(parent),
    m_isNeedToWork(true)
{
    m_txid = txid;
}

CheckoutTransaction::~CheckoutTransaction()
{

}

void CheckoutTransaction::startTask()
{
    std::string strErr;
    int index = 0;
    bool isFinished = false;
    while(isNeedToWork()){
        if(CheckoutTransaction::isTransactionFinished(m_txid,strErr)){
            isFinished = true;
            break;
        }
        QThread::sleep(2);
        Q_EMIT updateTaskTime(++index);
    }
    if(isFinished)
        Q_EMIT checkTransactionFinished();
    else
    {
        Q_EMIT threadStopped(); 
    }
}

bool CheckoutTransaction::isTransactionFinished(std::string txid,std::string& strErr)
{
    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txid)];  //watch only not check

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
    if(!fLocked && confirms < 1){
        strErr = "The transaction not confirms: "+std::to_string(confirms);
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec %s\n",strErr);
        return false;
    }
    if(wtx.HasCreatedService()){
        strErr = "The transaction has been used";
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec current %s\n",strErr);
        return false;
    }
    LogPrintf("AddDockerServiceDlg::is Transaction Finished\n");
    return true;
}

AskDNDataWorker::AskDNDataWorker(QObject* parent) :
    QObject(parent)
{

}

AskDNDataWorker::~AskDNDataWorker()
{

}

void AskDNDataWorker::startTask()
{
    LogPrintf("AskDNDataWorker::startTask Ask.\n");

    int index = 0;
    bool isTaskFinished = false;
    while(isNeedToWork()){
        if(isAskDNDataFinished()){
            isTaskFinished = true;
            break;
        }
        QThread::sleep(1);
        Q_EMIT updateTaskTime(++index);
        if(index >= LOADRESOURCETIMEOUT)
            break;
    }
    Q_EMIT askDNDataFinished(isTaskFinished);
}

bool AskDNDataWorker::isAskDNDataFinished()
{
    if(dockerServerman.getDNDataStatus() == CDockerServerman::Ask){
        LogPrintf("AddDockerServiceDlg get DNData Status:CDockerServerman::Asking\n");
        return false;
    }
    else if(dockerServerman.getDNDataStatus() == CDockerServerman::Received ||
            dockerServerman.getDNDataStatus() == CDockerServerman::Free){
        LogPrintf("AskDNDataWorker -> isAskDNDataFinished\n");
        return true;
    }
}
