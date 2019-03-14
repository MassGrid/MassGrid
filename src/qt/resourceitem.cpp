#include "resourceitem.h"
#include "ui_resourceitem.h"

ResourceItem::ResourceItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ResourceItem)
{
    ui->setupUi(this);
    connect(ui->buyBtn,SIGNAL(clicked()),this,SIGNAL(sig_buyClicked()));

}

ResourceItem::~ResourceItem()
{
    delete ui;
}

void ResourceItem::loadResourceData(const QString& gpuName,const QString& gpuCount,const QString& romType,const QString& romCount,
                          const QString& cpuName,const QString& cpuCount,const QString& availibleCount,const QString& amount)
{
    ui->label_gpuName->setText(gpuName);
    ui->label_gpuCount->setText(gpuCount);
    ui->label_romType->setText(romType);
    ui->label_romCount->setText(romCount);
    ui->label_cpuName->setText(cpuName);
    ui->label_cpuCount->setText(cpuCount);
    ui->label_availibleCount->setText(availibleCount);
    ui->label_amount->setText(amount);
}

QString ResourceItem::getGPUName()const
{
    return ui->label_gpuName->text();
}

QString ResourceItem::getGPUCount()const
{
    return ui->label_gpuCount->text();
}

QString ResourceItem::getRomType()const
{
    return ui->label_romType->text();
}

QString ResourceItem::getRomCount()const
{
    return ui->label_romCount->text();
}

QString ResourceItem::getCPUName()const
{
    return ui->label_cpuName->text();
}

QString ResourceItem::getCPUCount()const
{
    return ui->label_cpuCount->text();
}

QString ResourceItem::getAvailibleCount()const
{
    return ui->label_availibleCount->text();
}

QString ResourceItem::getAmount()const
{
    return ui->label_amount->text();
}
