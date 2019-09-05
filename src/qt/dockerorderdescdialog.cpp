#include "dockerorderdescdialog.h"
#include "ui_dockerorderdescdialog.h"
#include "guiutil.h"
#include "dockerordertablemodel.h"

#include <QModelIndex>
#include <QSettings>
#include <QString>
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include <QFontMetrics>
#include <QFont>
#include <stdlib.h>
#include <QDateTime>
#include "guiutil.h"
#include "util.h"

extern CWallet* pwalletMain;

DockerOrderDescDialog::DockerOrderDescDialog(WalletModel *model,QWidget *parent) :
    QDialog(parent),
    m_walletModel(model),
    ui(new Ui::DockerOrderDescDialog),
    m_mousePress(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));

    ui->reletTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->reletTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->reletTableWidget->verticalHeader()->setVisible(false); 
    connect(ui->tabWidget, SIGNAL(currentChanged(int)),this,SLOT(slot_curTabPageChanged(int)));

    ui->label_titlename->setText(tr("Docker order Detail"));
    this->setAttribute(Qt::WA_TranslucentBackground);
    ui->label_37->hide();
    ui->label_taskstate->hide();
    GUIUtil::MakeShadowEffect(this,ui->centerWin);

    ui->tabWidget->setCurrentIndex(0);
}

DockerOrderDescDialog::~DockerOrderDescDialog()
{
    delete ui;
}

void DockerOrderDescDialog::mousePressEvent(QMouseEvent *e)
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

void DockerOrderDescDialog::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void DockerOrderDescDialog::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void DockerOrderDescDialog::setwalletTx(const std::string& txid,CWalletTx& wtx)
{
    ui->label_serviceid->setText(QString::fromStdString(wtx.Getserviceid()));
    ui->label_verison->setText(QString::fromStdString(wtx.Getverison()));
    ui->label_createtime->setText(QDateTime::fromTime_t(atoi(wtx.Getcreatetime())).toString("yyyy-MM-dd hh:mm:ss"));
    ui->label_deletetime->setText(QDateTime::fromTime_t(atoi(wtx.Getdeletetime())).toString("yyyy-MM-dd hh:mm:ss"));

    CAmount amount_int = (CAmount)QString::fromStdString(wtx.Getprice()).toDouble();
    QString amount = MassGridUnits::formatHtmlWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), amount_int) + "/H";
    // m_createOutPoint = wtx.Getmasternodeoutpoint();
    m_createOutPoint = GUIUtil::getOutPoint(txid,wtx.Getmasternodeaddress()).ToStringShort();
    ui->label_price->setText(amount);

    // init address label
    QFont ft;
    QFontMetrics fm(ft);

    QString masternodeoutpointStr = fm.elidedText(QString::fromStdString(wtx.Getmasternodeoutpoint()), Qt::ElideRight, ui->label_masternodeoutpoint->width()*GUIUtil::GetDPIValue());
    QString custeraddressStr = fm.elidedText(QString::fromStdString(wtx.Getcusteraddress()), Qt::ElideRight, ui->label_custeraddress->width()*GUIUtil::GetDPIValue());
    QString provideraddressStr = fm.elidedText(QString::fromStdString(wtx.Getprovideraddress()), Qt::ElideRight, ui->label_provideraddress->width()*GUIUtil::GetDPIValue());
    QString masternodeaddressStr = fm.elidedText(QString::fromStdString(wtx.Getmasternodeaddress()), Qt::ElideRight, ui->label_masternodeaddress->width()*GUIUtil::GetDPIValue());
    QString tlementtxidStr = fm.elidedText(QString::fromStdString(wtx.Gettlementtxid()), Qt::ElideRight, ui->label_tlementtxid->width()*GUIUtil::GetDPIValue());

    ui->label_feerate->setText(QString::fromStdString(wtx.Getfeerate()));
    ui->label_cpuname->setText(QString::fromStdString(wtx.GetCPUType()));
    ui->label_cpucount->setText(QString::fromStdString(wtx.GetCPUThread()));
    ui->label_memname->setText(QString::fromStdString(wtx.GetMemoryType()));
    ui->label_memcount->setText(QString::fromStdString(wtx.GetMemoryCount()));
    ui->label_gpuname->setText(QString::fromStdString(wtx.GetGPUType()));
    ui->label_gpucount->setText(QString::fromStdString(wtx.GetGPUCount()));
    ui->label_masternodeip->setText(QString::fromStdString(wtx.Getmasternodeip()));
    ui->label_masternodeoutpoint->setText(masternodeoutpointStr);
    ui->label_custeraddress->setText(custeraddressStr);
    ui->label_provideraddress->setText(provideraddressStr);
    ui->label_masternodeaddress->setText(masternodeaddressStr);
    ui->label_tlementtxid->setText(tlementtxidStr);

    if(atoi(wtx.Gettaskstate()) != 8){

        QString taskstate = fm.elidedText(QString::fromStdString(wtx.Gettaskstatuscode()), Qt::ElideRight, ui->label_masternodeaddress->width());
        ui->label_taskstate->setText(taskstate);

        ui->label_37->show();
        ui->label_taskstate->show();
    }

    ui->label_orderstatus->setText(QString::fromStdString(wtx.Getstate())= "completed" ? tr("Settled") : tr("Paid"));
    loadRerentView();
}

void DockerOrderDescDialog::loadRerentView()
{
    ui->reletTableWidget->setRowCount(0);

    if(m_walletModel == NULL)
        return ;
    std::list<std::string> txidList = m_walletModel->getDockerOrderTableModel()->getRerentTxidList();

    std::list<std::string>::iterator iter = txidList.begin();

    int count =0;
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

            CAmount reletHourTime = amount/nPrice;
            double tmp1 = amount%nPrice;
            double tmp2 = tmp1/nPrice;
            int reletMinTime = tmp2*60;

            QLabel *label = new QLabel(ui->reletTableWidget);
            label->setText(QString("+%1:%2").arg(QString::number(reletHourTime).sprintf("%02d",reletHourTime) ).arg(QString::number(reletMinTime).sprintf("%02d",reletMinTime)));
            label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            label->setStyleSheet("color:green;");

            ui->reletTableWidget->insertRow(count);
            ui->reletTableWidget->setItem(count, 0, txidItem);
            ui->reletTableWidget->setItem(count, 1, priceItem);
            ui->reletTableWidget->setItem(count, 2, amountItem);
            ui->reletTableWidget->setItem(count, 3, createtimeItem);
            ui->reletTableWidget->setCellWidget(count,4,label);

            for(int i=0;i<4;i++)
                ui->reletTableWidget->item(count,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);            
                count++;
        }
    }
}

void DockerOrderDescDialog::resizeEvent(QResizeEvent *event)
{
    resetTableWidgetTitle();
    QWidget::resizeEvent(event);
}

void DockerOrderDescDialog::resetTableWidgetTitle()
{
    int columnCount = 5; //= ui->reletTableWidget->columnCount();
    int itemwidth = ui->reletTableWidget->width()/columnCount;
    ui->reletTableWidget->setColumnWidth(0, itemwidth*1.2);
    ui->reletTableWidget->setColumnWidth(1, itemwidth*1.2);
    ui->reletTableWidget->setColumnWidth(2, itemwidth);
    ui->reletTableWidget->setColumnWidth(3, itemwidth*0.8);
    ui->reletTableWidget->setColumnWidth(4, itemwidth*0.8);
}

void DockerOrderDescDialog::slot_curTabPageChanged(int index)
{
    if(index == 1){
        resetTableWidgetTitle();
    }
}