#ifndef RESOURCEITEM_H
#define RESOURCEITEM_H

#include <QWidget>
#include "amount.h"

namespace Ui {
class ResourceItem;
}

class ResourceItem : public QWidget
{
    Q_OBJECT

public:
    explicit ResourceItem(QWidget *parent = 0);
    ~ResourceItem();

    void loadResourceData(const QString& gpuName,const QString& gpuCount,const QString& romType,const QString& romCount,
                          const QString& cpuName,const QString& cpuCount,const QString& availibleCount,const QString& amount);

    QString getGPUName()const;
    QString getGPUCount()const;
    QString getRomType()const;
    QString getRomCount()const;
    QString getCPUName()const;
    QString getCPUCount()const;
    QString getAvailibleCount()const;

    CAmount getAmount()const;
    void setAmount(CAmount );

private:
    Ui::ResourceItem *ui;

    CAmount m_amount;

Q_SIGNALS:
    void sig_buyClicked();
};

#endif // RESOURCEITEM_H
