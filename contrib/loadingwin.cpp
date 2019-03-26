#include "loadingwin.h"
#include "ui_loadingwin.h"

LoadingWin::LoadingWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadingWin)
{
    ui->setupUi(this);
}

LoadingWin::~LoadingWin()
{
    delete ui;
}
