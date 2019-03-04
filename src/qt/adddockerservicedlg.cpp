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

AddDockerServiceDlg::AddDockerServiceDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDockerServiceDlg),
    m_walletModel(NULL)
{
    ui->setupUi(this);

    // connect(ui->okButton, SIGNAL(clicked()), this, SLOT(slot_okbutton()));
    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(slot_close()));
    connect(ui->openPubKeyButton,SIGNAL(clicked()),this,SLOT(slot_openPubKeyFile()));
    
    ui->label_titleName->setText(tr("Create Service"));
    this->setAttribute(Qt::WA_TranslucentBackground);

    ui->spinBox_gpucount->setEnabled(false);
    ui->spinBox_memorybyte->setEnabled(false);
    ui->spinBox_cpucount->setEnabled(false);

    connect(ui->nextButton_1,SIGNAL(clicked()),this,SLOT(slot_nextStep()));
    connect(ui->nextButton_2,SIGNAL(clicked()),this,SLOT(slot_nextStep()));
    connect(ui->nextButton_3,SIGNAL(clicked()),this,SLOT(slot_nextStep()));
    connect(ui->nextButton_4,SIGNAL(clicked()),this,SLOT(slot_nextStep()));

    ui->stackedWidget->setCurrentIndex(0);
    connect(ui->comboBox_image_2,SIGNAL(currentIndexChanged(int)),this,SLOT(slot_imageCurrentChanged(int)));

    ui->spinBox_gpucount->setValue(0);
    ui->spinBox_memorybyte->setValue(0);
    ui->spinBox_cpucount->setValue(0);
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

bool AddDockerServiceDlg::slot_okbutton()
{
    // if(createDockerService()){
    //     accept();
    //     LogPrintf("---->slot_okbutton: create docer service sucess!");
    // }
    // else{
    //     CMessageBox::information(this, tr("Error"), tr("create docker service error"));
    //     close();
    // }
    return createDockerService();
}

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

        if(btnRetVal == CMessageBox::Cancel)
            return;
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

void AddDockerServiceDlg::slot_nextStep()
{
    int index = ui->stackedWidget->currentIndex();

    switch (index)
    {
        case 0 :
            if(!doStep1()){
                return ;
            }
            ui->label_step2->setEnabled(true);
            ui->line_2->setEnabled(true);
            ui->label_14->setEnabled(true);
            break;
        case 1 : 
            if(!doStep2()){
                return ;
            }
            ui->line_3->setEnabled(true);
            ui->label_step3->setEnabled(true);
            ui->label_15->setEnabled(true);
            break;
        case 2 :
            if(!doStep3()){
                return ;
            }
            ui->label_step4->setEnabled(true);
            ui->label_17->setEnabled(true);
            break;
        case 3 :
            if(!doStep4()){
                return ;
            }
            close();
            return;    
        default:
            break;
    }
    ui->stackedWidget->setCurrentIndex(index+1);
    showStep(index+1);
}

bool AddDockerServiceDlg::doStep1()
{
    if(ui->spinBox_gpucount->value() > 0 && ui->spinBox_memorybyte->value() > 0 && ui->spinBox_cpucount->value() > 0 ){
        ui->lineEdit__TotalCoin->setText(ui->lineEdit_totalCoin->text());
        return true;
    }
    else
    {
        CMessageBox::information(this, tr("错误"), tr("配置不能为空！"));
        return false;
    }
}

bool AddDockerServiceDlg::doStep2()
{
    ui->nextButton_3->setEnabled(false);
    QTimer::singleShot(1000,this,SLOT(slot_timeOut()));
    return true;
}

bool AddDockerServiceDlg::doStep3()
{
    return slot_okbutton();
    // return true;
}

bool AddDockerServiceDlg::doStep4()
{
    // slot_okbutton();
    // return true;
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

    DockerCreateService createService{};

    std::string strssh_pubkey = ui->textEdit_sshpubkey->toPlainText().toStdString().c_str();
    createService.ssh_pubkey = strssh_pubkey;

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

    createService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
    createService.txid = uint256();
    
    std::string strServiceName = ui->lineEdit_name->text().toStdString().c_str();
    createService.serviceName = strServiceName;

    std::string strServiceImage = QString("massgrid/" + ui->comboBox_image->currentText()).toStdString().c_str();

    createService.image = strServiceImage;

    int64_t strServiceCpu = ui->spinBox_cpucount->value()*DOCKER_CPU_UNIT;
    createService.item.cpu.Count = strServiceCpu;
    LogPrintf("cpu %lld \n",createService.item.cpu.Count);
    int64_t strServiceMemoey_byte = ui->spinBox_memorybyte->value()*DOCKER_MEMORY_UNIT;
    createService.item.mem.Count = strServiceMemoey_byte;
    
    std::string strServiceGpuName = ui->comboBox_gpuname->currentText().toStdString().c_str();
    createService.item.gpu.Name = strServiceGpuName;

    int64_t strServiceGpu = ui->spinBox_gpucount->value();
    createService.item.gpu.Count = strServiceGpu;

    std::string strn2n_Community = ui->lineEdit_n2n_name->text().toStdString().c_str();
    createService.n2n_community = strn2n_Community;

    if(!dockercluster.CreateAndSendSeriveSpec(createService)){
        LogPrintf("dockercluster.CreateAndSendSeriveSpec error\n");
        // CMessageBox::information(this, tr("Docker option"), tr("Create and send SeriveSpec failed!"));
        return false;
    }
    return true;
}

void AddDockerServiceDlg::slot_imageCurrentChanged(int index)
{
    LogPrintf("slot_imageCurrentChanged index:%d\n",index);
    if(index == 0){
        ui->spinBox_gpucount->setValue(0);
        ui->spinBox_memorybyte->setValue(0);
        ui->spinBox_cpucount->setValue(0);
        ui->lineEdit_totalCoin->setText("0");
    }
    else if(index == 1){
        ui->spinBox_gpucount->setValue(2);
        ui->spinBox_memorybyte->setValue(2);
        ui->spinBox_cpucount->setValue(1);
        ui->lineEdit_totalCoin->setText("10");
    }
    else{
        ui->spinBox_gpucount->setValue(8);
        ui->spinBox_memorybyte->setValue(8);
        ui->spinBox_cpucount->setValue(4);
        ui->lineEdit_totalCoin->setText("20");
    }
}

void AddDockerServiceDlg::showStep(int index)
{

}

void AddDockerServiceDlg::slot_timeOut()
{
    static int index = 20;
    int total = ui->lineEdit__TotalCoin->text().toInt();
    if(total != 10){
        if(index > 0 ){
            ui->label_timeout->setText(QString::number(--index));
            QTimer::singleShot(1000,this,SLOT(slot_timeOut()));
            return ;
        }
        else{
            index = 20;
            disconnect(ui->nextButton_3,SIGNAL(clicked()),this,SLOT(slot_nextStep()));
            connect(ui->nextButton_3,SIGNAL(clicked()),this,SLOT(doStep2()));
            CMessageBox::information(this, tr("错误"), tr("服务创建请求超时，请等待1分钟后重新操作!"));
            ui->nextButton_3->setEnabled(true);
            ui->nextButton_3->setText("重新创建");
            index = 20;
        }
    }
    else{
        if(index > 10 ){
            ui->label_timeout->setText(QString::number(--index));
            QTimer::singleShot(1000,this,SLOT(slot_timeOut()));
            return ;
        }
        else{
            index = 20;
            slot_nextStep();
        }
    }
}
