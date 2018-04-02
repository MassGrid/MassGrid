#include "privkeymgr.h"
#include "ui_privkeymgr.h"
#include "rpcconsole.h"
#include "cmessagebox.h"
#include "util.h"

PrivKeyMgr::PrivKeyMgr(bool inputMode,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrivKeyMgr)
{
    ui->setupUi(this);

    QString retCommand;
    int category = 0;
    QString command = QString("dumpprivkey address");
    RPCConsole::RunCommand(command,category,retCommand);
    QString errorMsg = QString("Please enter the wallet passphrase with walletpassphrase first");

    if(category == RPCConsole::CMD_ERROR && retCommand.contains(errorMsg)){
        // ui->stackedWidget->setCurrentIndex(0);
        changeCurrentPage(0);
    }
    else{
        if(inputMode){
            // ui->stackedWidget->setCurrentIndex(2);
            // ui->label_titleName->setText(tr("Import private key"));
            // ui->okButton->setText(tr("Import"));
            changeCurrentPage(2);
        }
        else{
            // ui->stackedWidget->setCurrentIndex(1);
            // ui->label_titleName->setText(tr("Dump private key"));
            // ui->okButton->setText(tr("Generate"));
            changeCurrentPage(1);
        }
    }

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    this->setAttribute(Qt::WA_TranslucentBackground);
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
            command = QString("walletpassphrase %1 60").arg(ui->lineEdit_password->text());
            RPCConsole::RunCommand(command,category,retCommand);
            if(category == RPCConsole::CMD_REPLY){
                if(inputMode){
                    // ui->stackedWidget->setCurrentIndex(2);
                    // ui->okButton->setText(tr("Generate"));
                    changeCurrentPage(2);
                }
                else{
                    // ui->stackedWidget->setCurrentIndex(1);
                    changeCurrentPage(1);
                }
                return ;
            }
        }
        break;
        case 1:
        {
            command = QString("dumpprivkey %1").arg(ui->lineEdit_walletAddre->text());
            RPCConsole::RunCommand(command,category,retCommand);
            if(category == RPCConsole::CMD_REPLY){
                ui->lineEdit_privKey->setText(retCommand);
                return ;
            }
        }
        break;
        case 2:
        {
            command = QString("importprivkey %1").arg(ui->lineEdit_privKey_2->text());
            RPCConsole::RunCommand(command,category,retCommand);
            if(category == RPCConsole::CMD_REPLY){
                ui->lineEdit_privKey->setText(retCommand);
                QDialog::accept();
            }
        }
        break;
    }
    if(category == RPCConsole::CMD_ERROR){
        CMessageBox::information(this, tr("Error command"),retCommand);
    }
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