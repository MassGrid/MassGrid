#include "simplesendcoindlg.h"
#include "ui_simplesendcoindlg.h"

SimpleSendcoinDlg::SimpleSendcoinDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimpleSendcoinDlg)
{
    ui->setupUi(this);
}

SimpleSendcoinDlg::~SimpleSendcoinDlg()
{
    delete ui;
}
