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

AddDockerServiceDlg::AddDockerServiceDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDockerServiceDlg),
    m_walletModel(NULL)
{
    ui->setupUi(this);

    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(slot_okbutton()));
    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->openPubKeyButton,SIGNAL(clicked()),this,SLOT(slot_openPubKeyFile()));
    
    ui->label_titleName->setText(tr("Create Service"));
    this->setAttribute(Qt::WA_TranslucentBackground);

    ui->spinBox_gpucount->setEnabled(false);
    ui->spinBox_memorybyte->setEnabled(false);
    ui->spinBox_cpucount->setEnabled(false);
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

void AddDockerServiceDlg::slot_okbutton()
{
    if(createDockerService()){
        accept();
    }
    else{
        CMessageBox::information(this, tr("Error"), tr("create docker service error"));
        close();
    }
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
    // throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
    if(!dockercluster.ProcessDockernodeConnections()){
        LogPrintf("Connect to Masternode failed!\n");
         return false;
    }

    DockerCreateService createService{};

    createService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
    createService.vin = CTxIn();
    
    std::string strServiceName = ui->lineEdit_name->text().toStdString().c_str();
    createService.serviceName = strServiceName;

    std::string strServiceImage = QString("massgrid/" + ui->comboBox_image->currentText()).toStdString().c_str();

    createService.image = strServiceImage;

    int64_t strServiceCpu = ui->spinBox_cpucount->value()*DOCKER_CPU_UNIT;
    createService.cpu = strServiceCpu;
    LogPrintf("cpu %lld \n",createService.cpu);
    int64_t strServiceMemoey_byte = ui->spinBox_memorybyte->value()*DOCKER_MEMORY_UNIT;
    createService.memory_byte = strServiceMemoey_byte;
    
    std::string strServiceGpuName = ui->comboBox_gpuname->currentText().toStdString().c_str();
    createService.gpuname = strServiceGpuName;

    int64_t strServiceGpu = ui->spinBox_gpucount->value();
    createService.gpu = strServiceGpu;

    std::string strn2n_Community = ui->lineEdit_n2n_name->text().toStdString().c_str();
    createService.n2n_community = strn2n_Community;

    std::string strssh_pubkey = ui->textEdit_sshpubkey->toPlainText().toStdString().c_str();
    // "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDwKzxP+YJHSU/qgT8X79HnktF8Kpkec7cUEDGkyqwQXOhLMUG2XDDOqQAsRIHjCuCgP0fi8oeYO+/h+c/su6L5sAzs0zMXFUkAYHowe0OpPEVFXkSfd2rGbnFGVyRec2LuzN63X92WNycvG/TP7WobBizp1CXQDGEouSHw38kRYPRnr93YPVDJ6GUwlqEND35WiAEFpQ3n9CbYMiX+Eg3ItVXjXJc9R63oLwKGn9Ko4UDfpHqKhGNJ5KQ2LPIevhlbuP9rm7hCjoqx0krBJxfXVwlGTZE3hpteMpcZPdAKPcyHBx6P/YLEQHqiUNaGMF3hWtIr3CJqDDOMmKj70KOt oasis@xiejiataodeMacBook-Pro.local";
    createService.ssh_pubkey = strssh_pubkey;
    

    if(!dockercluster.CreateAndSendSeriveSpec(createService)){
        LogPrintf("dockercluster.CreateAndSendSeriveSpec error\n");
        CMessageBox::information(this, tr("Docker option"), tr("Create and send SeriveSpec failed!"));
        return false;
    }
    CMessageBox::information(this, tr("Docker option"), tr("Create and send SeriveSpec success!"));
    return true;
}
