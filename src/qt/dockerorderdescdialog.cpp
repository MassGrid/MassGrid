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

    ui->label_titlename->setText(tr("Docer order Detail"));
    this->setAttribute(Qt::WA_TranslucentBackground);
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
    int frameendy = framey+30;
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

void DockerOrderDescDialog::setwalletTx(CWalletTx& wtx)
{
    ui->label_serviceid->setText(QString::fromStdString(wtx.Getserviceid()));
    ui->label_verison->setText(QString::fromStdString(wtx.Getverison()));
    ui->label_createtime->setText(QString::fromStdString(wtx.Getcreatetime()));
    ui->label_deletetime->setText(QString::fromStdString(wtx.Getdeletetime()));

    CAmount amount_int = (CAmount)QString::fromStdString(wtx.Getprice()).toDouble();
    QString amount = MassGridUnits::formatHtmlWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), amount_int) + "/H";

    ui->label_price->setText(amount);

    ui->label_feerate->setText(QString::fromStdString(wtx.Getfeerate()));
    ui->label_cpuname->setText(QString::fromStdString(wtx.Getcpuname()));
    ui->label_cpucount->setText(QString::fromStdString(wtx.Getcpucount()));
    ui->label_memname->setText(QString::fromStdString(wtx.Getmemname()));
    ui->label_memcount->setText(QString::fromStdString(wtx.Getmemcount()));
    ui->label_gpuname->setText(QString::fromStdString(wtx.Getgpuname()));
    ui->label_gpucount->setText(QString::fromStdString(wtx.Getgpucount()));
    ui->label_custeraddress->setText(QString::fromStdString(wtx.Getcusteraddress()));
    ui->label_provideraddress->setText(QString::fromStdString(wtx.Getprovideraddress()));
    ui->label_taskstate->setText(QString::fromStdString(wtx.Gettaskstate()));
    ui->label_tlementtxid->setText(QString::fromStdString(wtx.Gettlementtxid()));
    ui->label_masternodeaddress->setText(QString::fromStdString(wtx.Getmasternodeaddress()));
    ui->label_masternodeip->setText(QString::fromStdString(wtx.Getmasternodeip()));
    ui->label_masternodeoutpoint->setText(QString::fromStdString(wtx.Getmasternodeoutpoint()));
    ui->label_orderstatus->setText(QString::fromStdString(wtx.Getorderstatus())= "1" ? tr("Settled") : tr("Paid"));
}