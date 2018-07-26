// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/massgrid-config.h"
#endif

#include "addressbookpage.h"
#include "ui_addressbookpage.h"

#include "addresstablemodel.h"
#include "massgridgui.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "util.h"
#include "optionsmodel.h"

#include <QIcon>
#include <QMenu>
#include "cmessagebox.h"
#include <QSortFilterProxyModel>

AddressBookPage::AddressBookPage(Mode mode, Tabs tab, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookPage),
    model(0),
    mode(mode),
    tab(tab),
    m_mousePress(false)
{
    ui->setupUi(this);
    switch(mode)
    {
    case ForSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Choose the address to send coins to")); break;
        case ReceivingTab: setWindowTitle(tr("Choose the address to receive coins with")); break;
        case MainAddressTab: setWindowTitle(tr("Choose the address as the default receive address")); break;        
        }
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setFocus();
        ui->closeButton->setText(tr("C&hoose"));
        ui->exportButton->hide();
        break;
    case ForEditing:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Sending addresses")); break;
        case ReceivingTab: setWindowTitle(tr("Receiving addresses")); break;
        case MainAddressTab: setWindowTitle(tr("Receiving addresses"));break;
        }
        break;
    }
    switch(tab)
    {
    case SendingTab:
        ui->labelExplanation->setText(tr("These are your MassGrid addresses for sending payments. Always check the amount and the receiving address before sending coins."));
        ui->deleteAddress->setVisible(true);
        ui->okButton->setVisible(false);
        break;
    case ReceivingTab:
        ui->labelExplanation->setText(tr("These are your MassGrid addresses for receiving payments. It is recommended to use a new receiving address for each transaction."));
        ui->deleteAddress->setVisible(false);
        ui->newAddress->setVisible(false);
        ui->okButton->setVisible(false);
        ui->copyAddress->setVisible(false);
        break;
    case MainAddressTab:
        disconnect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->labelExplanation->setText(tr("These are your MassGrid addresses for receiving payments. It is recommended to use a new receiving address for each transaction."));
        ui->deleteAddress->setVisible(false);
        ui->newAddress->setEnabled(false);
        ui->closeButton->hide();
        ui->exportButton->hide();
        break;
    }

    // Context menu actions
    QAction *copyAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy &Label"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);
    QAction *setMianAddrAction = new QAction(tr("&Set Main Address"), this);
    deleteAction = new QAction(ui->deleteAddress->text(), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(editAction);
    if(tab == SendingTab)
        contextMenu->addAction(deleteAction);
    contextMenu->addSeparator();

    // Connect signals for context menu actions
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(on_copyAddress_clicked()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(onCopyLabelAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(onEditAction()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(on_deleteAddress_clicked()));

    connect(setMianAddrAction, SIGNAL(triggered()), this, SLOT(setMainAddress()));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    ui->label_titleName->setText(this->windowTitle());
    this->setAttribute(Qt::WA_TranslucentBackground);
}

AddressBookPage::~AddressBookPage()
{
    delete ui;
}

void AddressBookPage::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    switch(tab)
    {
    case ReceivingTab:
        // Receive filter
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Receive);
        break;
    case SendingTab:
        // Send filter
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Send);
        break;
    case MainAddressTab:
        // Receive filter
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Receive);
     break;
    }
    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionChanged()));

    // Select row for newly created address
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));

    selectionChanged();
}

void AddressBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}

void AddressBookPage::onCopyLabelAction()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}

void AddressBookPage::onEditAction()
{
    if(!model)
        return;

    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg(
        tab == SendingTab ?
        EditAddressDialog::EditSendingAddress :
        EditAddressDialog::EditReceivingAddress, 0);

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressBookPage::on_newAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(
        tab == SendingTab ?
        EditAddressDialog::NewSendingAddress :
        EditAddressDialog::NewReceivingAddress, 0);
    dlg.setModel(model);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);
    if(dlg.exec())
    {
        newAddressToSelect = dlg.getAddress();
    }
}

void AddressBookPage::on_deleteAddress_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}
void AddressBookPage::setMainAddress()
{
    QTableView *table = ui->tableView;
    int curRow = table->currentIndex().row();

    if(curRow<0){
        int rowCount = table->model()->rowCount();

        if(rowCount<0){
            return ;
        }
        else{
            curRow = 0;
        }
    }

    QString address = ui->tableView->model()->index(curRow,1).data().toString();
    // ui->label_mainAddress->setText(tr("(Main address:%1)").arg(address));


    WalletModel* walletModel=this->model->getWalletModel();

    if(walletModel != 0){
        OptionsModel* optionsModel = walletModel->getOptionsModel();
        optionsModel->setMainAddress(address);
    }
}

void AddressBookPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        switch(tab)
        {
        case SendingTab:
            // In sending tab, allow deletion of selection
            ui->deleteAddress->setEnabled(true);
            ui->deleteAddress->setVisible(true);
            deleteAction->setEnabled(true);
            ui->copyAddress->setEnabled(true);
            break;
        case ReceivingTab:
            // Deleting receiving addresses, however, is not allowed
            ui->deleteAddress->setEnabled(false);
            ui->deleteAddress->setVisible(false);
            deleteAction->setEnabled(true); 
            ui->newAddress->setEnabled(false); 
            break;
        case MainAddressTab: break;
        }
        // ui->copyAddress->setEnabled(true);
    }
    else
    {
        ui->deleteAddress->setEnabled(false);
        ui->copyAddress->setEnabled(false);
    }
}

void AddressBookPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    Q_FOREACH (const QModelIndex& index, indexes) {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void AddressBookPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Address List"), QString(),
        tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);

    if(!writer.write()) {
        CMessageBox::information(0, tr("Exporting Failed"),
            tr("There was an error trying to save the address list to %1. Please try again.").arg(filename));
    }

}

void AddressBookPage::on_okButton_clicked()
{
    QTableView *table = ui->tableView;
    int curRow = table->currentIndex().row();
    if(curRow>=0){
        setMainAddress();
        accept();
    }
}

void AddressBookPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void AddressBookPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect))
    {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}
void AddressBookPage::mousePressEvent(QMouseEvent *e)
{
    int posx = e->pos().x();
    int posy = e->pos().y();
    int framex = ui->mainframe->pos().x();
    int framey = ui->mainframe->pos().y();
    int frameendx = framex+ui->mainframe->width();
    int frameendy = framey+30;
    if(posx>framex && posx<frameendx && posy>framey && posy<frameendy){
        m_mousePress = true;
        m_last = e->globalPos();
    }
    else{
        m_mousePress = false;
    }
}

void AddressBookPage::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void AddressBookPage::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}
