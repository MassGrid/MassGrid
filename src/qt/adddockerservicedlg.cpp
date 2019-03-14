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

extern SendCoinsDialog* g_sendCoinsPage;

AddDockerServiceDlg::AddDockerServiceDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDockerServiceDlg),
    m_walletModel(NULL)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(slot_close()));
    connect(ui->openPubKeyButton,SIGNAL(clicked()),this,SLOT(slot_openPubKeyFile()));
    
    ui->label_titleName->setText(tr("Create Service"));
    this->setAttribute(Qt::WA_TranslucentBackground);

    connect(ui->nextButton_2,SIGNAL(clicked()),this,SLOT(doTransaction()));
    connect(ui->nextButton_3,SIGNAL(clicked()),this,SLOT(doStep3()));
    connect(ui->nextButton_4,SIGNAL(clicked()),this,SLOT(doStep4()));

    ui->stackedWidget->setCurrentIndex(0);
    initTableWidget();
    askForDNData();
    QTimer::singleShot(200,this,SLOT(initTableWidget()));

}

AddDockerServiceDlg::~AddDockerServiceDlg()
{
    delete ui;
}

void AddDockerServiceDlg::mousePressEvent(QMouseEvent *e)
{
    int posx = e->pos().x();
    int posy = e->pos().y();
    int framex = ui->mainframe->pos().x();
    int framey = ui->mainframe->pos().y();
    int frameendx = framex+ui->mainframe->width();
    int frameendy = framey+30;
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

//转账页面开始，关闭的时候将创建服务的数据添加到创建服务的队列中，可以完成转账完成后的自动创建服务，前端加提示
//创建服务失败时，可以引导用户回去重选配置
//订单管理可以跳转到创建服务中
void AddDockerServiceDlg::slot_close()
{
    if(ui->stackedWidget->currentIndex() == 1){
        CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("退出"),
        tr("正在转账中，退出后可以在订单管理中查询转账结果并进行下一步操作，确认关闭？"),
        CMessageBox::Ok_Cancel, CMessageBox::Cancel);

        if(btnRetVal == CMessageBox::Cancel)
            return;
    }
    else if(ui->stackedWidget->currentIndex() == 2){
        CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("退出"),
        tr("转账已完成，服务创建中，现在退出将在15-60分钟内收到退款，是否仍旧退出？"),
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


// do send coin option
//点击发送过后，开始10s倒计时检查转账状态，同时打开loading页面
void AddDockerServiceDlg::doStep2()
{
    ui->nextButton_2->setEnabled(false);
    QTimer::singleShot(1000,this,SLOT(slot_refreshTransactionStatus()));
}

void AddDockerServiceDlg::doTransaction()
{
    if(!sendCoin())
        return ;

    doStep2();
}

void AddDockerServiceDlg::slot_refreshTransactionStatus()
{
    static int index = 20;
    if(index > 0 ){
        ui->label_refreshPayStatus->setText(QString::number(--index));
        QTimer::singleShot(1000,this,SLOT(slot_refreshTransactionStatus()));
        return ;
    }
    else{
        index = 20;
        std::string strErr;
        if(!isTransactionFinished(strErr)){
            CMessageBox::information(this, tr("Transaction Error"),QString::fromStdString(strErr));
            ui->label_refreshPayStatus->setText(QString::number(index));
            QTimer::singleShot(1000,this,SLOT(slot_refreshTransactionStatus()));
            return ;
        }
        slot_nextStep();
        doStep3();
    }
}

void AddDockerServiceDlg::doStep3()
{
    ui->nextButton_3->setEnabled(false);
    if(createDockerService()){
        // dockercluster.AskForDNData();
        QTimer::singleShot(3000,this,SLOT(refreshDNData()));
    }
    else
    {
        ui->nextButton_3->setText("重新创建");
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
    ui->textEdit_sshpubkey->setText(file.readAll());
    
}

void AddDockerServiceDlg::setaddr_port(const std::string& addr_port)
{
    m_addr_port = addr_port; 
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

    std::string strssh_pubkey = ui->textEdit_sshpubkey->text().toStdString().c_str();
    m_createService.ssh_pubkey = strssh_pubkey;

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
            dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);

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

    std::string strn2n_Community = ui->lineEdit_n2n_name->text().toStdString().c_str();
    m_createService.n2n_community = strn2n_Community;

    if(!dockercluster.CreateAndSendSeriveSpec(m_createService)){
        LogPrintf("dockercluster.CreateAndSendSeriveSpec error\n");
        return false;
    }
    // createService,mdndata.strErr
    return true;
}

bool AddDockerServiceDlg::isTransactionFinished(std::string& strErr)
{
    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(m_txid)];  //watch only not check

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
    LogPrint("docker","current transaction fLocked %d confirms %d\n",fLocked,confirms);
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
    return true;
}

void AddDockerServiceDlg::refreshDNData()
{
    static int index = 10;
    if(dockerServerman.getDNDataStatus() == CDockerServerman::Creating){
        if(index-- >0)
            QTimer::singleShot(2000,this,SLOT(refreshDNData()));
        else
        {
            CMessageBox::information(this, tr("Create Service Error"),tr("Can't receive create service feedback!"));
        }
    }
    else if(dockerServerman.getDNDataStatus() == CDockerServerman::Received ||
            dockerServerman.getDNDataStatus() == CDockerServerman::Free){

        std::map<std::string,Service> serverlist = dockercluster.dndata.mapDockerServiceLists;
        std::map<std::string,Service>::iterator iter = serverlist.begin();

        for(;iter != serverlist.end();iter++){
            QString id = QString::fromStdString(iter->first);
            Service service = serverlist[iter->first];

            if(m_txid == service.txid.ToString()){
                slot_nextStep();
                return ;
            }
            // QString name = QString::fromStdString(service.spec.name);
            // map<std::string,Task> mapDockerTasklists = service.mapDockerTaskLists;
        }
        if(dockercluster.dndata.errCode == 7){
            QString errStr = getErrorMsg(dockercluster.dndata.errCode);
            CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Create Failed"),
                tr("转账已完成，资源不足，是否重选资源或者进入订单管理系统申请退款？"),
                CMessageBox::Ok_Cancel, CMessageBox::Cancel);

            if(btnRetVal == CMessageBox::Cancel){
                close();
            }
            else{
                gotoStep1Page();
            }
        }
    }
}

    // SUCCESS = 0,
    // SIGTIME_ERROR,
    // VERSION_ERROR,
    // CHECKSIGNATURE_ERROR,
    // NO_THRANSACTION,
    // TRANSACTION_NOT_CONFIRMS,
    // TRANSACTION_DOUBLE_CREATE,
    // SERVICEITEM_NOT_FOUND,
    // SERVICEITEM_NO_RESOURCE,
    // PAYMENT_NOT_ENOUGH,
    // GPU_AMOUNT_ERROR,
    // CPU_AMOUNT_ERROR,
    // MEM_AMOUNT_ERROR,
    // TRANSACTION_DOUBLE_TLEMENT,
    // PUBKEY_ERROR

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
    ui->stackedWidget->setCurrentIndex(0);
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
}

void AddDockerServiceDlg::checkCreateTaskStatus(std::string txid)
{

}

void AddDockerServiceDlg::slot_timeOut()
{
    static int index = 20;
    if(index > 0 ){
        ui->label_timeout->setText(QString::number(--index));
        QTimer::singleShot(1000,this,SLOT(slot_timeOut()));
        return ;
    }
    else{
        index = 20;
        std::string strErr;
        if(!isTransactionFinished(strErr)){
            CMessageBox::information(this, tr("Transaction Error"),QString::fromStdString(strErr));
            QTimer::singleShot(1000,this,SLOT(slot_timeOut()));
        }
        slot_nextStep();
    }
}

bool AddDockerServiceDlg::sendCoin()
{
    SendCoinsRecipient recipient;

    if(!validate(recipient))
        return false;
    if (recipient.paymentRequest.IsInitialized())
        return false;

    // Normal payment
    recipient.address = ui->payTo->text();
    // recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();

    // getValue(recipient);
    QList<SendCoinsRecipient> recipients;
    recipients.append(recipient);
    std::string txid = g_sendCoinsPage->send(recipients,"","",true);
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

void AddDockerServiceDlg::askForDNData()
{
    dockercluster.AskForDNData();
    refreshServerList();
}

void AddDockerServiceDlg::refreshServerList()
{
    static int refreshCount = 0 ;

    if(dockerServerman.getDNDataStatus() == CDockerServerman::Ask){
        // if(MasternodeList::DockerUpdateMode::WhenNormal)
        QTimer::singleShot(2000,this,SLOT(refreshServerList()));
        LogPrintf("MasternodeList get DNData Status:CDockerServerman::Ask\n");
        return ;
    }
    else if(dockerServerman.getDNDataStatus() == CDockerServerman::Received ||
            dockerServerman.getDNDataStatus() == CDockerServerman::Free){
        loadResourceData();
        LogPrintf("MasternodeList get DNData Status:CDockerServerman::Received\n");
        return ;
    }
}

void AddDockerServiceDlg::initTableWidget()
{
    // ui->tableWidget_resource->hideColumn(0);
    ui->tableWidget_resource->verticalHeader()->setVisible(false);

    ui->tableWidget_resource->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_resource->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_resource->verticalHeader()->setVisible(false); 
    ui->tableWidget_resource->horizontalHeader()->setVisible(false); 
    ui->tableWidget_resource->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_resource->setAlternatingRowColors(true);

    int itemwidth = ui->tableWidget_resource->width()/8;
    ui->tableWidget_resource->setColumnWidth(0,ui->tableWidget_resource->width());
}

void AddDockerServiceDlg::loadResourceData()
{
    std::map<Item,Value_price> items = dockercluster.dndata.items;
     m_masterndoeAddr = dockercluster.dndata.masternodeAddress;
    std::map<Item,Value_price>::iterator iter = items.begin();

    ui->tableWidget_resource->setRowCount(0);
    ui->tableWidget_resource->setColumnCount(1);

    int count = 0;

    for(;iter != items.end();iter++){
        ui->tableWidget_resource->setRowCount(count+1);
        ResourceItem * item = new ResourceItem(ui->tableWidget_resource);
        item->loadResourceData(QString::fromStdString(iter->first.gpu.Name),QString::number(iter->first.gpu.Count),
                               QString::fromStdString(iter->first.mem.Name),QString::number(iter->first.mem.Count),
                               QString::fromStdString(iter->first.cpu.Name),QString::number(iter->first.cpu.Count),
                               QString::number(iter->second.count),QString::number(iter->second.price));
        connect(item,SIGNAL(sig_buyClicked()),this,SLOT(slot_buyClicked()));

        item->resize(ui->tableWidget_resource->width(),item->height());
        ui->tableWidget_resource->setRowHeight(count,item->height());
        ui->tableWidget_resource->setCellWidget(count,0,item);
        count++;
    }
}

void AddDockerServiceDlg::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void AddDockerServiceDlg::slot_buyClicked()
{
    ResourceItem* item = dynamic_cast<ResourceItem *>(QObject::sender());
    doStep1(item);
    if(!m_txid.size()){
        return ;
    }

    std::string strErr;
    if(!isTransactionFinished(strErr)){
        CMessageBox::information(this, tr("Transaction Error"),QString::fromStdString(strErr));
        // ui->nextButton_2->setEnabled(true);
        return ;
    }
    slot_nextStep();
    doStep3();
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
    QString amount = item->getAmount();

    m_createService.item.cpu.Count = cpuCount.toInt();
    m_createService.item.cpu.Name = cpuName.toStdString().c_str();

    m_createService.item.mem.Count = romCount.toInt();
    m_createService.item.mem.Name = romType.toStdString().c_str();

    m_createService.item.gpu.Name = gpuName.toStdString().c_str();
    m_createService.item.gpu.Count = ((++index)%2) ? 0 : gpuCount.toInt();

    ui->payAmount->setValue(amount.toDouble());
    ui->payTo->setText(QString::fromStdString(m_masterndoeAddr));
    ui->payAmount->setFocus();

    slot_nextStep();
}