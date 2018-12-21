#include "adddockerservicedlg.h"
#include "ui_adddockerservicedlg.h"
#include "../dockercluster.h"
#include "../rpc/protocol.h"
#include "cmessagebox.h"

AddDockerServiceDlg::AddDockerServiceDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDockerServiceDlg)
{
    ui->setupUi(this);

    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(slot_okbutton()));
    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    ui->label_titleName->setText(this->windowTitle());
    this->setAttribute(Qt::WA_TranslucentBackground);
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
        CMessageBox::information(0, tr("Error"), tr("create docker service error"));
    }
}

void AddDockerServiceDlg::setaddr_port(const std::string& addr_port)
{
    m_addr_port = addr_port; 
}

bool AddDockerServiceDlg::createDockerService()
{
    if(!m_addr_port.size()){
        LogPrintf("connect docker address error!\n");
        return false ;
    }
    if(!dockercluster.SetConnectDockerAddress(m_addr_port))
        throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
    if(!dockercluster.ProcessDockernodeConnections())
        throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Connect to Masternode failed");

    DockerCreateService createService{};

    createService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
    createService.vin = CTxIn();
    
    std::string strServiceName = ui->lineEdit_name->text().toStdString().c_str();
    createService.serviceName = strServiceName;

    std::string strServiceImage = ui->comboBox_image->currentText().toStdString().c_str();

    createService.image = strServiceImage;

    int64_t strServiceCpu = ui->spinBox_cpucount->value();
    createService.cpu = strServiceCpu;
    LogPrintf("cpu %lld \n",createService.cpu);
    int64_t strServiceMemoey_byte = ui->spinBox_memorybyte->value();
    createService.memory_byte = strServiceMemoey_byte;
    
    std::string strServiceGpuName = ui->comboBox_gpuname->currentText().toStdString().c_str();
    createService.gpuname = strServiceGpuName;

    int64_t strServiceGpu = ui->spinBox_gpucount->value();
    createService.gpu = strServiceGpu;

    // QString n2n_name = ui->lineEdit_n2n_name->text();
    // if(n2n_name == "")
    //     n2n_name = "massgrid";
    std::string strn2n_Community = ui->lineEdit_n2n_name->text().toStdString().c_str();
    createService.n2n_community = strn2n_Community;

    std::string strssh_pubkey = ui->textEdit_sshpubkey->toPlainText().toStdString().c_str();
    // "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDwKzxP+YJHSU/qgT8X79HnktF8Kpkec7cUEDGkyqwQXOhLMUG2XDDOqQAsRIHjCuCgP0fi8oeYO+/h+c/su6L5sAzs0zMXFUkAYHowe0OpPEVFXkSfd2rGbnFGVyRec2LuzN63X92WNycvG/TP7WobBizp1CXQDGEouSHw38kRYPRnr93YPVDJ6GUwlqEND35WiAEFpQ3n9CbYMiX+Eg3ItVXjXJc9R63oLwKGn9Ko4UDfpHqKhGNJ5KQ2LPIevhlbuP9rm7hCjoqx0krBJxfXVwlGTZE3hpteMpcZPdAKPcyHBx6P/YLEQHqiUNaGMF3hWtIr3CJqDDOMmKj70KOt oasis@xiejiataodeMacBook-Pro.local";
    // ui->lineEdit_pubkey->text().toStdString().c_str();
    createService.ssh_pubkey = strssh_pubkey;
    
    if(!dockercluster.CreateAndSendSeriveSpec(createService)){
        LogPrintf("dockercluster.CreateAndSendSeriveSpec error\n");
        CMessageBox::information(0, tr("connect docker"), tr("dockercluster.CreateAndSendSeriveSpec error"));
        return false;
    }
    CMessageBox::information(0, tr("connect docker"), tr("createService.sspec sucess"));
    LogPrintf(" createService.sspec hash %s \n",createService.ToString());
    // return "dockercluster.CreateAndSendSeriveSpec successfully";
    return true;
}

// name
// image
// n2nName
// CPU count
// memory byte
// GPU name
// GPU count
