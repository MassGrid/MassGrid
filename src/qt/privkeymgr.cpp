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
        ui->stackedWidget->setCurrentIndex(0);
    }
    else{
        if(inputMode){
            ui->stackedWidget->setCurrentIndex(2);
            ui->label_titleName->setText(tr("Import private key"));
        }
        else{
            ui->stackedWidget->setCurrentIndex(1);
            ui->label_titleName->setText(tr("Dump private key"));
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

    LogPrintf("PrivKeyMgr button 1");

    switch(index){
        case 0:
        {
            LogPrintf("PrivKeyMgr button 2\n");
            command = QString("walletpassphrase %1 60").arg(ui->lineEdit_password->text());
            RPCConsole::RunCommand(command,category,retCommand);
            LogPrintf("PrivKeyMgr button 3\n");
            if(category == RPCConsole::CMD_REPLY){
                if(inputMode){
                    ui->stackedWidget->setCurrentIndex(2);
                    LogPrintf("PrivKeyMgr button 3-1\n");
                }
                else{
                    ui->stackedWidget->setCurrentIndex(1);
                    LogPrintf("PrivKeyMgr button 3-2\n");
                }

                LogPrintf("PrivKeyMgr button 4\n");

                return ;
            }
        }
        break;
        case 1:
        {
            LogPrintf("PrivKeyMgr button 5\n");
            command = QString("dumpprivkey %1").arg(ui->lineEdit_walletAddre->text());
            RPCConsole::RunCommand(command,category,retCommand);
            LogPrintf("PrivKeyMgr button 6\n");
            if(category == RPCConsole::CMD_REPLY){
                ui->lineEdit_privKey->setText(retCommand);
                LogPrintf("PrivKeyMgr button 7\n");

                return ;
            }
        }
        break;
        case 2:
        {
            LogPrintf("PrivKeyMgr button 8\n");
            command = QString("importprivkey %1").arg(ui->lineEdit_privKey_2->text());
            RPCConsole::RunCommand(command,category,retCommand);
            LogPrintf("PrivKeyMgr button 9\n");

            if(category == RPCConsole::CMD_REPLY){
                ui->lineEdit_privKey->setText(retCommand);
                QDialog::accept();
                LogPrintf("PrivKeyMgr button 10\n");
            }
        }
        break;
    }
    if(category == RPCConsole::CMD_ERROR){
        CMessageBox::information(this, tr("Error command"),retCommand);
    }
}
