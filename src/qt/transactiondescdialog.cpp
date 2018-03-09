// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactiondescdialog.h"
#include "ui_transactiondescdialog.h"

#include "transactiontablemodel.h"

#include <QModelIndex>

TransactionDescDialog::TransactionDescDialog(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransactionDescDialog)
{
    ui->setupUi(this);
    QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));

    ui->label_titlename->setText(this->windowTitle());
    this->setAttribute(Qt::WA_TranslucentBackground);
}

TransactionDescDialog::~TransactionDescDialog()
{
    delete ui;
}

void TransactionDescDialog::mousePressEvent(QMouseEvent *e)
{
    m_last = e->globalPos();
}

void TransactionDescDialog::mouseMoveEvent(QMouseEvent *e)
{
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(this->x()+dx, this->y()+dy);
}

void TransactionDescDialog::mouseReleaseEvent(QMouseEvent *e)
{
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(this->x()+dx, this->y()+dy);
}