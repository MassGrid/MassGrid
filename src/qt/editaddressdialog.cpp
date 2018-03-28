// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "editaddressdialog.h"
#include "ui_editaddressdialog.h"

#include "addresstablemodel.h"
#include "guiutil.h"

#include <QDataWidgetMapper>
#include "cmessagebox.h"

EditAddressDialog::EditAddressDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditAddressDialog),
    mapper(0),
    mode(mode),
    model(0)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->addressEdit, this);

    switch(mode)
    {
    case NewReceivingAddress:
        setWindowTitle(tr("New receiving address"));
        ui->addressEdit->setEnabled(false);
        break;
    case NewSendingAddress:
        setWindowTitle(tr("New sending address"));
        break;
    case EditReceivingAddress:
        setWindowTitle(tr("Edit receiving address"));
        ui->addressEdit->setEnabled(false);
        break;
    case EditSendingAddress:
        setWindowTitle(tr("Edit sending address"));
        break;
    }

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    ui->label_titleName->setText(this->windowTitle());
    this->setAttribute(Qt::WA_TranslucentBackground);
}

EditAddressDialog::~EditAddressDialog()
{
    delete ui;
}

void EditAddressDialog::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    mapper->setModel(model);
    mapper->addMapping(ui->labelEdit, AddressTableModel::Label);
    mapper->addMapping(ui->addressEdit, AddressTableModel::Address);
}

void EditAddressDialog::loadRow(int row)
{
    mapper->setCurrentIndex(row);//row
}

bool EditAddressDialog::saveCurrentRow()
{
    if(!model)
        return false;

    switch(mode)
    {
    case NewReceivingAddress:
    case NewSendingAddress:
        address = model->addRow(
                mode == NewSendingAddress ? AddressTableModel::Send : AddressTableModel::Receive,
                ui->labelEdit->text(),
                ui->addressEdit->text());
        break;
    case EditReceivingAddress:
    case EditSendingAddress:
        if(mapper->submit())
        {
            address = ui->addressEdit->text();
        }
        break;
    }
    return !address.isEmpty();
}

void EditAddressDialog::accept()
{
    if(!model)
        return;

    if(!saveCurrentRow())
    {
        switch(model->getEditStatus())
        {
        case AddressTableModel::OK:
            // Failed with unknown reason. Just reject.
            break;
        case AddressTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case AddressTableModel::INVALID_ADDRESS:
            CMessageBox::warning(this, windowTitle(),
                tr("The entered address \"%1\" is not a valid MassGrid address.").arg(ui->addressEdit->text()),
                CMessageBox::Ok, CMessageBox::Ok);
            break;
        case AddressTableModel::DUPLICATE_ADDRESS:
            CMessageBox::warning(this, windowTitle(),
                tr("The entered address \"%1\" is already in the address book.").arg(ui->addressEdit->text()),
                CMessageBox::Ok, CMessageBox::Ok);
            break;
        case AddressTableModel::WALLET_UNLOCK_FAILURE:
            CMessageBox::critical(this, windowTitle(),
                tr("Could not unlock wallet."),
                CMessageBox::Ok, CMessageBox::Ok);
            break;
        case AddressTableModel::KEY_GENERATION_FAILURE:
            CMessageBox::critical(this, windowTitle(),
                tr("New key generation failed."),
                CMessageBox::Ok, CMessageBox::Ok);
            break;

        }
        return;
    }
    QDialog::accept();
}

QString EditAddressDialog::getAddress() const
{
    return address;
}

void EditAddressDialog::setAddress(const QString &address)
{
    // this->address = address;
    // ui->addressEdit->setText(address);
}

void EditAddressDialog::mousePressEvent(QMouseEvent *e)
{
    m_last = e->globalPos();
}

void EditAddressDialog::mouseMoveEvent(QMouseEvent *e)
{
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void EditAddressDialog::mouseReleaseEvent(QMouseEvent *e)
{
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}