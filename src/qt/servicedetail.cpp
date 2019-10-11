#include "servicedetail.h"
#include "dockercluster.h"
#include "dockerserverman.h"
#include "ui_servicedetail.h"
#include <QDateTime>
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "massgridunits.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include <math.h>
#include "dockerordertablemodel.h"
#include "uint256.h"
#include "util.h"
#include "validation.h"

extern CWallet* pwalletMain;

// const char* strTaskStateTmp2[] = {"new", "allocated", "pending", "assigned",
//     "accepted", "preparing", "ready", "starting",
//     "running", "complete", "shutdown", "failed",
//     "rejected", "remove", "orphaned"};

ServiceDetail::ServiceDetail(QWidget* parent) 
    : QDialog(parent),
    ui(new Ui::ServiceDetail),
    m_walletModel(NULL)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setColumnWidth(0, 120);

    ui->rerentTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->rerentTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->rerentTableWidget->verticalHeader()->setVisible(false); 
    connect(ui->tabWidget, SIGNAL(currentChanged(int)),this,SLOT(slot_curTabPageChanged(int)));

    ui->label_titlename->setText(tr("Service Detail"));
    this->setAttribute(Qt::WA_TranslucentBackground);
    GUIUtil::MakeShadowEffect(this,ui->centerWin);

    ui->label_11->hide();
    ui->label_12->hide();
    ui->label_13->hide();
    ui->label_15->hide();

    ui->label_GPUName->hide();
    ui->label_cpuCount->hide();
    ui->label_memoryBytes->hide();
    ui->label_GPUCount->hide();
    ui->tabWidget->setCurrentIndex(0);
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

void ServiceDetail::setService(ServiceInfo& service)
{
    updateServiceDetail(service);
    std::vector<Task> mapDockerTasklists = service.TaskInfo;
    int taskStatus = -1;
    updateTaskDetail(mapDockerTasklists, taskStatus);
}

void ServiceDetail::updateServiceDetail(ServiceInfo& service)
{
    uint64_t createdAt = service.CreatedAt;
    std::string name = service.CreateSpec.ServiceName;

    bool fPersistentsore = false;

    std::string image = service.CreateSpec.Image;
    std::string userName = "root";//= service.CreateSpec;

    std::vector<Task> taskInfo = service.TaskInfo;
    std::vector<std::string> env;

    if(taskInfo.size()){
        env = taskInfo[0].spec.containerSpec.env;
    }

    int count = env.size();
    QString n2n_name;
    QString n2n_localip;
    QString n2n_SPIP;
    QString ssh_pubkey;
    for (int i = 0; i < count; i++) {
        QString envStr = QString::fromStdString(env[i]);
        ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);        
        ui->tableWidget->setItem(i,0,new QTableWidgetItem(envStr.split("=").at(0)));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(envStr.split("=").at(1)));
    }

    ui->label_name->setText(QString::fromStdString(name));
    ui->label_image->setText(QString::fromStdString(image));
    ui->label_user->setText(QString::fromStdString(userName));
    ui->label_PersistentStore->setText(fPersistentsore?tr("Yes"):tr("No"));

    m_createOutPoint = service.CreateSpec.OutPoint.ToStringShort();
    LOCK2(cs_main, pwalletMain->cs_wallet);
    CWalletTx& wtx = pwalletMain->mapWallet[service.CreateSpec.OutPoint.hash];
    
    std::vector<ServiceOrder> Order = service.Order;

    int orderSize = Order.size();
    int64_t totalRemainingTimeDuration = 0;
    for(int i=0;i<orderSize;i++){
        totalRemainingTimeDuration += Order[i].RemainingTimeDuration;
    }
    QString timeout = QDateTime::fromTime_t(service.LastCheckTime).addMSecs(totalRemainingTimeDuration/1000000).toString("yyyy-MM-dd hh:mm:ss");
    ui->label_serviceTimeout->setText(timeout);

    QString createAt = QDateTime::fromTime_t(service.CreatedAt).toString("yyyy-MM-dd hh:mm:ss");
    ui->label_createAt->setText(createAt);

    loadRerentView();
}

void ServiceDetail::setModel(WalletModel* model)
{
    m_walletModel = model;
}

void ServiceDetail::updateTaskDetail(std::vector<Task>& mapDockerTasklists, int& taskStatus)
{
    int count = mapDockerTasklists.size();
    for(int i=0;i<count;i++){
        Task task = mapDockerTasklists[i];
        QString name = QString::fromStdString(task.name);
        QString serviceID = QString::fromStdString(task.serviceID);
        int64_t slot = task.slot;

        taskStatus = task.status.state;
        QString taskStatusStr = GUIUtil::GetServiceTaskStatus(taskStatus);  // QString::fromStdString(strTaskStateTmp2[taskStatus]);

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

        ui->label_taskName->setText(QString::fromStdString(task.ID));
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

void ServiceDetail::loadRerentView()
{
    ui->rerentTableWidget->setRowCount(0);

    if(m_walletModel == NULL)
        return ;
    std::list<std::string> txidList = m_walletModel->getDockerOrderTableModel()->getRerentTxidList();

    std::list<std::string>::iterator iter = txidList.begin();

    int count =0;
    LOCK2(cs_main, pwalletMain->cs_wallet);
    for(;iter != txidList.end();iter++){
        std::string txidStr = *iter;
        CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr)];
        
        if(wtx.GetCreateOutPoint() == m_createOutPoint){
            QTableWidgetItem *txidItem = new QTableWidgetItem(QString::fromStdString(txidStr));

            CAmount nPrice = (CAmount)QString::fromStdString(wtx.Getprice()).toDouble();
            QString strPrice = MassGridUnits::formatWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), nPrice) + "/H";

            QString date;
            CAmount amount;
            m_walletModel->getDockerOrderTableModel()->getTransactionDetail(wtx,date,amount);
            amount = amount*-1;

            QTableWidgetItem *priceItem = new QTableWidgetItem(strPrice);
            QTableWidgetItem *amountItem = new QTableWidgetItem(MassGridUnits::formatWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), amount)); //wtx.GetAmounts()
            QTableWidgetItem *createtimeItem = new QTableWidgetItem(date);

            CAmount rerentHourTime = amount/nPrice;
            double tmp1 = amount%nPrice;
            double tmp2 = tmp1/nPrice;
            int rerentMinTime = tmp2*60;

            QLabel *label = new QLabel(ui->rerentTableWidget);
            label->setText(QString("+%1:%2").arg(QString::number(rerentHourTime).sprintf("%02d",rerentHourTime) ).arg(QString::number(rerentMinTime).sprintf("%02d",rerentMinTime)));
            label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            label->setStyleSheet("color:green;");

            ui->rerentTableWidget->insertRow(count);
            ui->rerentTableWidget->setItem(count, 0, txidItem);
            ui->rerentTableWidget->setItem(count, 1, priceItem);
            ui->rerentTableWidget->setItem(count, 2, amountItem);
            ui->rerentTableWidget->setItem(count, 3, createtimeItem);
            ui->rerentTableWidget->setCellWidget(count,4,label);

            for(int i=0;i<4;i++)
                ui->rerentTableWidget->item(count,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);            
                count++;
        }
    }
    ui->label_orderOutpoint->setText(QString::fromStdString(m_createOutPoint));
}

void ServiceDetail::resizeEvent(QResizeEvent *event)
{
    resetTableWidgetTitle();
    QWidget::resizeEvent(event);
}

void ServiceDetail::resetTableWidgetTitle()
{
    int columnCount = 5; //= ui->rerentTableWidget->columnCount();
    int itemwidth = ui->rerentTableWidget->width()/columnCount;
    ui->rerentTableWidget->setColumnWidth(0, itemwidth*1.2);
    ui->rerentTableWidget->setColumnWidth(1, itemwidth*1.2);
    ui->rerentTableWidget->setColumnWidth(2, itemwidth);
    ui->rerentTableWidget->setColumnWidth(3, itemwidth*0.8);
    ui->rerentTableWidget->setColumnWidth(4, itemwidth*0.8);

    // for(int i=0;i<columnCount;i++){
    //     ui->rerentTableWidget->setColumnWidth(i, itemwidth);
    // }
}

void ServiceDetail::slot_curTabPageChanged(int index)
{
    if(index == 1){
        resetTableWidgetTitle();
    }
}