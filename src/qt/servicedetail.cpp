#include "servicedetail.h"
#include "dockercluster.h"
#include "dockerserverman.h"
#include "ui_servicedetail.h"
#include <QDateTime>

const char* strTaskStateTmp2[] = {"new", "allocated", "pending", "assigned",
    "accepted", "preparing", "ready", "starting",
    "running", "complete", "shutdown", "failed",
    "rejected", "remove", "orphaned"};

ServiceDetail::ServiceDetail(QWidget* parent) : QDialog(parent),
                                                ui(new Ui::ServiceDetail)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    ui->label_titlename->setText(tr("Service Detail"));
    this->setAttribute(Qt::WA_TranslucentBackground);
}

ServiceDetail::~ServiceDetail()
{
    delete ui;
}

void ServiceDetail::mousePressEvent(QMouseEvent* e)
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

void ServiceDetail::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_mousePress)
        return;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x() + dx, this->y() + dy));
}

void ServiceDetail::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_mousePress)
        return;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x() + dx, this->y() + dy));
}

void ServiceDetail::setService(Service& service)
{
    updateServiceDetail(service);
    map<std::string, Task> mapDockerTasklists = service.mapDockerTaskLists;
    int taskStatus = -1;
    updateTaskDetail(mapDockerTasklists, taskStatus);
}

void ServiceDetail::updateServiceDetail(Service& service)
{
    uint64_t createdAt = service.createdAt;

    std::string name = service.spec.name;
    std::string address = service.spec.labels["com.massgrid.pubkey"];

    std::string image = service.spec.taskTemplate.containerSpec.image;
    std::string userName = service.spec.taskTemplate.containerSpec.user;

    std::vector<std::string> env = service.spec.taskTemplate.containerSpec.env;
    int count = env.size();
    QString n2n_name;
    QString n2n_localip;
    QString n2n_SPIP;
    QString ssh_pubkey;
    for (int i = 0; i < count; i++) {
        QString envStr = QString::fromStdString(env[i]);
        if (envStr.contains("N2N_NAME")) {
            n2n_name = envStr.split("=").at(1);
        } else if (envStr.contains("N2N_SERVERIP")) {
            n2n_localip = envStr.split("=").at(1);
        } else if (envStr.contains("N2N_SNIP")) {
            n2n_SPIP = envStr.split("=").at(1);
        } else if (envStr.contains("SSH_PUBKEY")) {
            int size = envStr.split(" ").size();
            if (size >= 2)
                ssh_pubkey = envStr.split(" ").at(1).mid(0, 10);
        }
    }

    ui->label_serviceTimeout->setText(QDateTime::fromTime_t(createdAt).addSecs(14400).toString("yyyy-MM-dd hh:mm:ss t"));
    ui->label_n2n_serverip->setText(n2n_SPIP);
    ui->label_n2n_name->setText(n2n_name);
    ui->label_n2n_localip->setText(n2n_localip);
    ui->label_ssh_pubkey->setText(ssh_pubkey);

    ui->label_name->setText(QString::fromStdString(name));
    ui->label_image->setText(QString::fromStdString(image));
    ui->label_user->setText(QString::fromStdString(userName));
}

void ServiceDetail::updateTaskDetail(std::map<std::string, Task>& mapDockerTasklists, int& taskStatus)
{
    map<std::string, Task>::iterator iter = mapDockerTasklists.begin();

    LogPrintf("mapDockerTasklists size:%d \n", mapDockerTasklists.size());

    for (; iter != mapDockerTasklists.end(); iter++) {
        std::string id = iter->first;
        Task task = iter->second;
        QString name = QString::fromStdString(task.name);
        QString serviceID = QString::fromStdString(task.serviceID);
        int64_t slot = task.slot;

        //std::string
        // int taskstatus = -1;
        taskStatus = task.status.state;
        QString taskStatusStr = QString::fromStdString(strTaskStateTmp2[taskStatus]);

        QString taskErr = QString::fromStdString(task.status.err);

        int64_t nanoCPUs = task.spec.resources.limits.nanoCPUs;
        int64_t memoryBytes = task.spec.resources.limits.memoryBytes;

        std::string gpuName;
        int64_t gpuCount = 0;

        int genericResourcesSize = task.spec.resources.reservations.genericResources.size();
        if (genericResourcesSize > 0) {
            gpuName = task.spec.resources.reservations.genericResources[0].discreateResourceSpec.kind;
            gpuCount = task.spec.resources.reservations.genericResources[0].discreateResourceSpec.value;
        }

        QString taskRuntime = QString::fromStdString(task.spec.runtime);

        ui->label_taskName->setText(QString::fromStdString(id));
        ui->label_cpuCount->setText(QString::number(nanoCPUs / DOCKER_CPU_UNIT));
        ui->label_memoryBytes->setText(QString::number(memoryBytes / DOCKER_MEMORY_UNIT));
        ui->label_GPUName->setText(QString::fromStdString(gpuName));
        ui->label_GPUCount->setText(QString::number(gpuCount));

        ui->label_taskStatus->setText(taskStatusStr);

        if (taskStatus == 8) {
            ui->label_16->setStyleSheet("color:green;");
            ui->label_taskStatus->setStyleSheet("color:green;");
        } else if (taskStatus != -1) {
            ui->label_16->setStyleSheet("color:red;");
            ui->label_taskStatus->setStyleSheet("color:red;");
        } else {
            ui->label_16->setStyleSheet("color:black;");
            ui->label_taskStatus->setStyleSheet("color:black;");
        }

        if (!taskErr.isEmpty()) {
            // ui->label_17->setVisible(true);
            // ui->textEdit_taskErr->setVisible(true);
            ui->label_17->show();
            ui->textEdit_taskErr->show();
            ui->textEdit_taskErr->setText(taskErr);
        } else {
            // ui->label_17->setVisible(false);
            // ui->textEdit_taskErr->setVisible(false);
            ui->label_17->hide();
            ui->textEdit_taskErr->hide();
        }
    }
}