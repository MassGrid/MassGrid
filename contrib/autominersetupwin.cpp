#include "autominersetupwin.h"
#include "ui_autominersetupwin.h"

AutoMinerSetupWin::AutoMinerSetupWin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoMinerSetupWin)
{
    ui->setupUi(this);
}

AutoMinerSetupWin::~AutoMinerSetupWin()
{
    delete ui;
}
