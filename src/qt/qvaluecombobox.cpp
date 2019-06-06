// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qvaluecombobox.h"
#include <QStyleFactory>

QValueComboBox::QValueComboBox(QWidget *parent) :
        QComboBox(parent), role(Qt::UserRole)
{
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(handleSelectionChanged(int)));

#ifdef Q_OS_MAC
    this->setStyle(QStyleFactory::create("Windows"));
#endif
    // this->setStyleSheet("QComboBox{\n    background-color:rgb(49, 61, 64); \n    color:white;\n    height:30px;\n    width:120px;\n    border:1px solid rgb(210,210,210);\n	border-radius:2px;\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n    width: 30px;\n    height:28px;\n    image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(49, 61, 64);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n  font:15pt;\n  background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
    this->setStyleSheet("QComboBox{\n    background-color:rgb(49, 61, 64); \n    color:white;\n   border:1px solid rgb(210,210,210);\n	border-radius:2px;\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n   image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(49, 61, 64);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n  font:15pt;\n  background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
}


QVariant QValueComboBox::value() const
{
    return itemData(currentIndex(), role);
}

void QValueComboBox::setValue(const QVariant &value)
{
    setCurrentIndex(findData(value, role));
}

void QValueComboBox::setRole(int role)
{
    this->role = role;
}

void QValueComboBox::handleSelectionChanged(int idx)
{
    Q_EMIT valueChanged();
}
