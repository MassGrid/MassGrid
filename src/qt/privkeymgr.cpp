#include "privkeymgr.h"
#include "ui_privkeymgr.h"
#include "rpcconsole.h"
#include "cmessagebox.h"
#include "util.h"
#include "massgridgui.h"
#include <QTimer>

PrivKeyMgr::PrivKeyMgr(bool inputMode,MassGridGUI *parentObj,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrivKeyMgr),
    m_inputMode(inputMode),
    m_mousePress(false)
{
    ui->setupUi(this);

    QString retCommand;
    int category = 0;
    QString command = QString("dumpprivkey address");
    RPCConsole::RunCommand(command,category,retCommand);
    QString errorMsg = QString("Please enter the wallet passphrase with walletpassphrase first");

    if(category == RPCConsole::CMD_ERROR && retCommand.contains(errorMsg)){
        changeCurrentPage(0);
    }
    else{
        if(m_inputMode){
            changeCurrentPage(2);
        }
        else{
            changeCurrentPage(1);
        }
    }

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    this->setAttribute(Qt::WA_TranslucentBackground);

    m_guiObj = parentObj;
}

PrivKeyMgr::~PrivKeyMgr()
{
    delete ui;
}

void PrivKeyMgr::on_okButton_clicked()
{
    int index = ui->stackedWidget->currentIndex();
    QString command,retCommand;
    int category = 0;

    switch(index){
        case 0:
        {
            QString password = ui->lineEdit_password->text();
            if(password.isEmpty())
                return ;
            command = QString("walletpassphrase %1 60").arg(password);
            RPCConsole::RunCommand(command,category,retCommand);
            if(category == RPCConsole::CMD_REPLY){
                if(m_inputMode){
                    changeCurrentPage(2);
                }
                else{
                    changeCurrentPage(1);
                }
                return ;
            }
        }
        break;
        case 1:
        {
            QString walletAddre = ui->lineEdit_walletAddre->text();
            if(walletAddre.isEmpty())
                return ;
            command = QString("dumpprivkey %1").arg(walletAddre);
            // emit sgl_startToImportPrivkey("",0);
            RPCConsole::RunCommand(command,category,retCommand);
            if(category == RPCConsole::CMD_REPLY){
                ui->lineEdit_privKey->setText(retCommand);
                return ;
            }
            else if(category == RPCConsole::CMD_ERROR){
                CMessageBox::information(this, tr("Error command"),retCommand);
            }
        }
        break;
        case 2:
        {
            QString privkey = ui->lineEdit_privKey_2->text();
            if(privkey.isEmpty())
                return ;
            QTimer::singleShot(200,this,SLOT(slot_RunCMD()));

            // command = QString("importprivkey %1").arg(privkey);
            m_guiObj->showProgress("",0);
            // RPCConsole::RunCommand(command,category,retCommand);
            // if(category == RPCConsole::CMD_REPLY){
            //     ui->lineEdit_privKey->setText(retCommand);
            //     QDialog::accept();
            // }
            // m_guiObj->showProgress("",100);
        }
        break;
    }
    // if(category == RPCConsole::CMD_ERROR){
    //     CMessageBox::information(this, tr("Error command"),retCommand);
    // }
}

void PrivKeyMgr::on_stackedWidget_currentChanged(int index)
{
    switch(index)
    {
        case 0: ui->okButton->setText(tr("Ok"));
            break;
        case 1: ui->okButton->setText(tr("Generate"));
            break;
        case 2: ui->okButton->setText(tr("Import"));
            break;
    }
}

void PrivKeyMgr::changeCurrentPage(int index)
{
    switch(index){
        case 0: ui->okButton->setText(tr("Ok"));
                ui->label_titleName->setText(tr("Enter the password"));
                ui->stackedWidget->setCurrentIndex(0);
            break;
        case 1: ui->okButton->setText(tr("Generate"));
                ui->label_titleName->setText(tr("Dump private key"));
                ui->stackedWidget->setCurrentIndex(1);
            break;
        case 2: ui->okButton->setText(tr("Import"));
                ui->label_titleName->setText(tr("Import private key"));
                ui->stackedWidget->setCurrentIndex(2);
            break;
    }
}

int timeCount =0;
void PrivKeyMgr::slot_RunCMD()
{
    QString command,retCommand;
    int category = 0;

    QString privkey = ui->lineEdit_privKey_2->text();

    command = QString("importprivkey %1").arg(privkey);
    m_guiObj->showProgress("",10);
    timeCount = 10;
    QTimer timer;
    connect(&timer,SIGNAL(timeout()),this,SLOT(slot_timeOut()));
    timer.start(400);
    RPCConsole::RunCommand(command,category,retCommand);
    if(category == RPCConsole::CMD_REPLY){
        m_guiObj->showProgress("",100);
        ui->lineEdit_privKey->setText(retCommand);
        QDialog::accept();
    }
    else if(category == RPCConsole::CMD_ERROR){
        timer.stop();
        m_guiObj->closeProgress();
        CMessageBox::information(this, tr("Error command"),retCommand);
    }
}

void PrivKeyMgr::slot_timeOut()
{
    if(timeCount >= 99)
        return ;
    m_guiObj->showProgress("",++timeCount);
}

void PrivKeyMgr::mousePressEvent(QMouseEvent *e)
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

void PrivKeyMgr::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void PrivKeyMgr::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

