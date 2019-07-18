#include "autominersetupwin.h"
#include "ui_autominersetupwin.h"
#include "guiutil.h"

AutoMinerSetupWin::AutoMinerSetupWin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoMinerSetupWin),
    m_mousePress(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->pBtn_ok,SIGNAL(clicked()),this,SLOT(slot_confirmSetup()));
    connect(ui->pBtn_defaultSetup,SIGNAL(clicked()),this,SLOT(slot_fillAutoMinerSetup()));
    connect(ui->pBtn_clear,SIGNAL(clicked()),this,SLOT(slot_clearSetup()));
    this->setAttribute(Qt::WA_TranslucentBackground);
    // inittableWidget();
    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

AutoMinerSetupWin::~AutoMinerSetupWin()
{
    delete ui;
}

void AutoMinerSetupWin::slot_fillAutoMinerSetup()
{
    ui->lineEdit_worker->setText(ui->lineEdit_worker->placeholderText());
    ui->lineEdit_minerpool->setText(ui->lineEdit_minerpool->placeholderText());
    ui->lineEdit_minertype->setText(ui->lineEdit_minertype->placeholderText());
}

void AutoMinerSetupWin::slot_clearSetup()
{
    ui->lineEdit_worker->setText("");
    ui->lineEdit_minerpool->setText("");
    ui->lineEdit_minertype->setText("");
    ui->lineEdit_address->setText("");
}

void AutoMinerSetupWin::slot_confirmSetup()
{
    if(ui->lineEdit_address->text().isEmpty()){
        CMessageBox::information(this, tr("Setup error"),tr("Miner address is empty!"));
        return ;
    }
    else if(ui->lineEdit_worker->text().isEmpty()){
        CMessageBox::information(this, tr("Setup error"),tr("Miner worker is empty!"));
        return ;
    }
    else if(ui->lineEdit_minerpool->text().isEmpty()){
        CMessageBox::information(this, tr("Setup error"),tr("Miner pool is empty!"));
        return ;
    }
    else if(ui->lineEdit_minertype->text().isEmpty()){
        CMessageBox::information(this, tr("Setup error"),tr("Miner type is empty!"));
        return;
    }
    std::map<std::string,std::string> env{};

    m_env["MINER_ADDRESS"] = ui->lineEdit_address->text().toStdString();
    m_env["MINER_WORKER"] = ui->lineEdit_worker->text().toStdString();
    m_env["MINER_POOL"] = ui->lineEdit_minerpool->text().toStdString();
    m_env["MINER_TYPE"] = ui->lineEdit_minertype->text().toStdString();

    accept();
}

std::map<std::string,std::string> AutoMinerSetupWin::getEnvSetup()
{
    return m_env;
}

// void AutoMinerSetupWin::inittableWidget()
// {
//     ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
//     // ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
//     ui->tableWidget->verticalHeader()->setVisible(false);
//     ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
//     ui->tableWidget->setColumnWidth(0, 150);
//     ui->tableWidget->setColumnWidth(0, 150);


//     ui->tabWidget->tabBar()->hide();
// }

// void AutoMinerSetupWin::slot_addPathRow()
// {
//     ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);

//     QLineEdit *edit1 = new QLineEdit();
//     QLineEdit *edit2 = new QLineEdit();
    
//     edit1->setStyleSheet("QLineEdit\n{   \n    border-color: rgb(186, 186, 186);\n    border:1px solid rgb(186, 186, 186);\n	background-color:rgba(255,255,255,0);\n	border-radius:5px;\n	margin:2px 2px 2px 2px;\n}\n\n");
//     edit2->setStyleSheet("QLineEdit\n{   \n    border-color: rgb(186, 186, 186);\n    border:1px solid rgb(186, 186, 186);\n	background-color:rgba(255,255,255,0);\n	border-radius:5px;\n	margin:2px 2px 2px 2px;\n}\n\n");

//     ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,0,edit1);
//     ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,1,edit2);
//     ui->tableWidget->setCellWidget(ui->tableWidget->rowCount()-1,2,new QPushButton(tr("Remove")));
//     ui->tableWidget->setRowHeight(ui->tableWidget->rowCount()-1,40);
// }

void AutoMinerSetupWin::mousePressEvent(QMouseEvent *e)
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

void AutoMinerSetupWin::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void AutoMinerSetupWin::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

