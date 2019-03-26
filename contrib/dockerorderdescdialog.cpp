#include "dockerorderdescdialog.h"
#include "ui_dockerorderdescdialog.h"

DockerOrderDescDialog::DockerOrderDescDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DockerOrderDescDialog)
{
    ui->setupUi(this);
}

DockerOrderDescDialog::~DockerOrderDescDialog()
{
    delete ui;
}
