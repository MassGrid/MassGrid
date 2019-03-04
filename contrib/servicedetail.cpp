#include "servicedetail.h"
#include "ui_servicedetail.h"

ServiceDetail::ServiceDetail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ServiceDetail)
{
    ui->setupUi(this);
}

ServiceDetail::~ServiceDetail()
{
    delete ui;
}
