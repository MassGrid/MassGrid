// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dockerorderview.h"

#include "addresstablemodel.h"
#include "massgridunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "dockerorderdescdialog.h"
#include "dockerorderfilterproxy.h"
#include "dockerorderrecord.h"
#include "dockerordertablemodel.h"
#include "walletmodel.h"
#include "massgridgui.h"
#include "ui_interface.h"
#include "init.h"
#include "definecalendar.h"
#include "dockercluster.h"
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSettings>
#include <QSignalMapper>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

/** Date format for persistence */
#include <QPushButton>
#include <QPainter>
#include <QListView>
#include <QComboBox>
#include <QStyleFactory>

#define SYNCTRANSTINEOUT 30
static const char* PERSISTENCE_DATE_FORMAT = "yyyy-MM-dd";

QMap<std::string,QPushButton*> m_mapViewBtns;

DockerOrderView::DockerOrderView(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent), model(0), dockerorderProxyModel(0),
    dockerorderView(0), abandonAction(0), columnResizingFixer(0),
    m_syncTransactionThread(NULL)
{
    QSettings settings;
    // Build filter row
    setContentsMargins(0,0,0,0);

    dockerorderView = 0;
    typeWidget = 0;
    dateWidget = 0;
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
    if (platformStyle->getUseExtraSpacing()) {
        hlayout->setSpacing(0);
        hlayout->addSpacing(6);
    } else {
        hlayout->setSpacing(1);
        hlayout->addSpacing(5);
    }
    watchOnlyWidget = new QComboBox(this);
    watchOnlyWidget->setFixedWidth(24);
    watchOnlyWidget->addItem("", DockerOrderFilterProxy::WatchOnlyFilter_All);
    watchOnlyWidget->addItem(QIcon(":/icons/eye_plus"), "", DockerOrderFilterProxy::WatchOnlyFilter_Yes);
    watchOnlyWidget->addItem(QIcon(":/icons/eye_minus"), "", DockerOrderFilterProxy::WatchOnlyFilter_No);
    hlayout->addWidget(watchOnlyWidget);
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    view->setShowGrid(false);
    // vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateRangeWidget());
    vlayout->addWidget(view);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
    vlayout->setMargin(0);
    if (platformStyle->getUseExtraSpacing()) {
        hlayout->addSpacing(width+2);
    } else {
        hlayout->addSpacing(width);
    }
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    view->installEventFilter(this);

    dockerorderView = view;

    // Actions
    abandonAction = new QAction(tr("Abandon transaction"), this);
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *copyTxHexAction = new QAction(tr("Copy raw transaction"), this);
    QAction *copyTxPlainText = new QAction(tr("Copy full transaction details"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);
    QAction *deleteServiceAction = new QAction(tr("Delete Service"), this);

    contextMenu = new QMenu(this);
    // contextMenu->addAction(copyAddressAction);
    // contextMenu->addAction(copyLabelAction);
    // contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(copyTxHexAction);
    contextMenu->addAction(copyTxPlainText);
    contextMenu->addAction(showDetailsAction);
    contextMenu->addAction(deleteServiceAction);
    contextMenu->addSeparator();
    contextMenu->addAction(abandonAction);
    // contextMenu->addAction(editLabelAction);

    mapperThirdPartyTxUrls = new QSignalMapper(this);

    // Connect actions
    connect(mapperThirdPartyTxUrls, SIGNAL(mapped(QString)), this, SLOT(openThirdPartyTxUrl(QString)));

    // connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    // connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(watchOnlyWidget, SIGNAL(activated(int)), this, SLOT(chooseWatchonly(int)));
    // connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    // connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));

    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(view, SIGNAL(clicked(QModelIndex)), this, SIGNAL(itemClicked(QModelIndex)));

    // connect(view, SIGNAL(clicked(QModelIndex)), this, SLOT(computeSum()));
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(abandonAction, SIGNAL(triggered()), this, SLOT(abandonTx()));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(copyTxHexAction, SIGNAL(triggered()), this, SLOT(copyTxHex()));
    connect(copyTxPlainText, SIGNAL(triggered()), this, SLOT(copyTxPlainText()));
    connect(editLabelAction, SIGNAL(triggered()), this, SLOT(editLabel()));
    // connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
    connect(deleteServiceAction, SIGNAL(triggered()), this, SLOT(deleteService()));
}

void DockerOrderView::setSearchWidget(QComboBox* dateComboBox,QComboBox* typeComboBox,QLineEdit* addrLineEdit)
{
    dateWidget = dateComboBox;
    typeWidget = typeComboBox;
    // addressWidget = addrLineEdit;

   dateWidget->setStyleSheet("QComboBox{border:0px; background-color:rgb(172,99,43); \
                    color:white; height:24px; width:40px;}\
                    QComboBox::down-arrow{\
                    border:hidden;\
                    background-color:rgb(172,99,430);\
                    border-image:url(:/res/pic/al.png);\
                    background-color:rgba(255,255,255,0);}\
                    QComboBox::drop-down{width:14px; border:0px;}\
                    QComboBox QAbstractItemView {\
                    color:rgb(255,255,255);\
                    border: 0px solid rgb(172,99,43);\
                    background-color:rgb(198, 125, 26);\
                    selection-color:white;\
                    selection-background-color: rgb(239,169,4);}\
                    QComboBox QAbstractItemView::item{\
                        height: 35px;\
                        background-color: rgb(198, 125, 26);\
                        border:hidden;\
                        color: rgb(255, 255, 255);\
                    }");


    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);

    typeWidget->addItem(tr("All"), DockerOrderFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), DockerOrderFilterProxy::TYPE(DockerOrderRecord::RecvWithAddress) |
                                        DockerOrderFilterProxy::TYPE(DockerOrderRecord::RecvFromOther));
    typeWidget->addItem(tr("Sent to"), DockerOrderFilterProxy::TYPE(DockerOrderRecord::SendToAddress) |
                                  DockerOrderFilterProxy::TYPE(DockerOrderRecord::SendToOther));
    typeWidget->addItem(tr("To yourself"), DockerOrderFilterProxy::TYPE(DockerOrderRecord::SendToSelf));
    typeWidget->addItem(tr("Mined"), DockerOrderFilterProxy::TYPE(DockerOrderRecord::Generated));
    typeWidget->addItem(tr("Other"), DockerOrderFilterProxy::TYPE(DockerOrderRecord::Other));



#if QT_VERSION >= 0x040700
    addrLineEdit->setPlaceholderText(tr("Enter address or label to search"));
#endif
}
void DockerOrderView::setModel(WalletModel *model)
{
    QSettings settings;
    this->model = model;
    if(model)
    {
        dockerorderProxyModel = new DockerOrderFilterProxy(this);
        dockerorderProxyModel->setSourceModel(model->getDockerOrderTableModel());
        dockerorderProxyModel->setDynamicSortFilter(true);
        dockerorderProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        dockerorderProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        dockerorderProxyModel->setSortRole(Qt::EditRole);

        dockerorderView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        dockerorderView->setModel(dockerorderProxyModel);
        dockerorderView->setAlternatingRowColors(true);
        dockerorderView->setSelectionBehavior(QAbstractItemView::SelectRows);
        dockerorderView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        dockerorderView->setSortingEnabled(true);
        dockerorderView->sortByColumn(DockerOrderTableModel::Status, Qt::DescendingOrder);
        dockerorderView->verticalHeader()->hide();

        dockerorderView->setColumnWidth(DockerOrderTableModel::Status, STATUS_COLUMN_WIDTH);
        dockerorderView->setColumnWidth(DockerOrderTableModel::Watchonly, WATCHONLY_COLUMN_WIDTH);
        dockerorderView->setColumnWidth(DockerOrderTableModel::Date, DATE_COLUMN_WIDTH);
        dockerorderView->setColumnWidth(DockerOrderTableModel::TxID, TYPE_COLUMN_WIDTH);
        dockerorderView->setColumnWidth(DockerOrderTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);
        dockerorderView->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        // Note: it's a good idea to connect this signal AFTER the model is set
        // connect(dockerorderView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(computeSum()));
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(dockerorderView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this);

        if (model->getOptionsModel())
        {
            // Add third party transaction URLs to context menu
            QStringList listUrls = model->getOptionsModel()->getThirdPartyTxUrls().split("|", QString::SkipEmptyParts);
            for (int i = 0; i < listUrls.size(); ++i)
            {
                QString host = QUrl(listUrls[i].trimmed(), QUrl::StrictMode).host();
                if (!host.isEmpty())
                {
                    QAction *thirdPartyTxUrlAction = new QAction(host, this); // use host as menu item label
                    if (i == 0)
                        contextMenu->addSeparator();
                    contextMenu->addAction(thirdPartyTxUrlAction);
                    connect(thirdPartyTxUrlAction, SIGNAL(triggered()), mapperThirdPartyTxUrls, SLOT(map()));
                    mapperThirdPartyTxUrls->setMapping(thirdPartyTxUrlAction, listUrls[i].trimmed());
                }
            }
        }
        // show/hide column Watch-only
        updateWatchOnlyColumn(model->haveWatchOnly());

        // Watch-only signal
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyColumn(bool)));

        connect(model->getDockerOrderTableModel(),SIGNAL(updateOrderStatus(const std::string&)),
                    this,SLOT(updateOrderStatus(const std::string&)));

        connect(model->getDockerOrderTableModel(),SIGNAL(addOperationBtn(int)),this,SLOT(addOperationBtn(int)));
        connect(model->getDockerOrderTableModel(),SIGNAL(deleteTransaction(int)),this,SLOT(addOperationBtn(int)));
        
        // Update transaction list with persisted settings
        // chooseType(settings.value("transactionType").toInt());
        // chooseDate(settings.value("transactionDate").toInt());

        int rowCount = dockerorderView->model()->rowCount();
        for(int i=0;i<rowCount;i++){
            std::string txidStr = dockerorderView->model()->index(i,DockerOrderTableModel::TxID).data().toString().toStdString();
            CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr)];  //watch only not check
            std::string orderstatusStr = wtx.Getorderstatus();
            std::string serviceidStr = wtx.Getserviceid();

            QString btnText;
            bool isNeedUpdateTD = getOrderBtnText(wtx,btnText);

            QPushButton *btn = new QPushButton(btnText,NULL);

            m_mapViewBtns[txidStr] = btn;
            btn->setStyleSheet("QPushButton\n{\n	background-color:rgb(239, 169, 4); color:rgb(255,255,255);\n	border-radius:2px;\n margin:2px; margin-left:6px;margin-right:6px;\n}");
            dockerorderView->setIndexWidget(dockerorderView->model()->index(i,DockerOrderTableModel::Operation),btn);
            connect(btn,SIGNAL(clicked(bool)),this,SLOT(slot_btnClicked()));
        }
    }
}

void DockerOrderView::addOperationBtn(int index)const
{
    std::string txidStr = dockerorderView->model()->index(index,DockerOrderTableModel::TxID).data().toString().toStdString();
    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr)];  //watch only not check
    QString btnText;
    bool isNeedUpdateTD = getOrderBtnText(wtx,btnText);
    QPushButton *btn = new QPushButton(btnText,dockerorderView);
    m_mapViewBtns[txidStr] = btn;

    btn->setStyleSheet("QPushButton\n{\n	background-color:rgb(239, 169, 4); color:rgb(255,255,255);\n	border-radius:2px;\n margin:2px; margin-left:6px;margin-right:6px;\n}");
    dockerorderView->setIndexWidget(dockerorderView->model()->index(index,DockerOrderTableModel::Operation),btn);
    connect(btn,SIGNAL(clicked(bool)),this,SLOT(slot_btnClicked()));
}

void DockerOrderView::deleteTransaction(int index)const
{
    std::string txidStr = dockerorderView->model()->index(index,DockerOrderTableModel::TxID).data().toString().toStdString();
    QPushButton* btn = m_mapViewBtns[txidStr];
    btn->setEnabled(false);
}

void DockerOrderView::updateAllOperationBtn()
{
    int count = dockerorderView->model()->rowCount();
    for(int i=0;i<count;i++){
        std::string txidStr = dockerorderView->model()->index(i,DockerOrderTableModel::TxID).data().toString().toStdString();
        bool isNeedUpdateTD = updateOrderStatus(txidStr);
        if(isNeedUpdateTD)
            updateTransData(QString::fromStdString(txidStr));
    }
}

bool DockerOrderView::updateOrderStatus(const std::string &txidStr)const
{
    QPushButton *btn = m_mapViewBtns[txidStr];

    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr)];  //watch only not check
    QString btnText;
    bool isNeedUpdateTD = getOrderBtnText(wtx,btnText);
    btn->setText(btnText);
    
    if(isNeedUpdateTD){
        LogPrintf("DockerOrderView::updateOrderStatus isNeedUpdateTD:%d\n",isNeedUpdateTD);
        LogPrintf("DockerOrderView::updateOrderStatus updateTransData:%s\n",txidStr);
    }

    return isNeedUpdateTD;
}

void DockerOrderView::slot_btnClicked()
{
    QPushButton *btn = dynamic_cast<QPushButton *>(QObject::sender());
    QModelIndex index = dockerorderView->indexAt(btn->pos());
    QString txidStr = dockerorderView->model()->index(index.row(),DockerOrderTableModel::TxID).data().toString();

    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr.toStdString())];  //watch only not check

    std::string orderstatusStr = wtx.Getorderstatus();
    std::string serviceidStr = wtx.Getserviceid();
    std::string masternodeip = wtx.Getmasternodeip();
    if(orderstatusStr == "1"){
        showOrderDetail(wtx);
    }
    else if(orderstatusStr == "0" && serviceidStr.size()){
        Q_EMIT openServicePage(masternodeip);
    }
    else if(orderstatusStr == "0" && !serviceidStr.size()){
        Q_EMIT gotoCreateServicePage(masternodeip,txidStr.split("-").at(0).toStdString());
    }
    else
    {
        //tr("Get Detail");
    }
}

bool DockerOrderView::getOrderBtnText(CWalletTx& wtx,QString& btnText)const
{
    std::string orderstatusStr = wtx.Getorderstatus();
    std::string serviceidStr = wtx.Getserviceid();
    // QString btnText; //= tr("Get Detail");
    if(orderstatusStr == "1")
        btnText = tr("Order Detail");
    else if(orderstatusStr == "0" && serviceidStr.size())
        btnText = tr("Service Detail");
    else if(orderstatusStr == "0" && !serviceidStr.size())
        btnText = tr("Create Service");
    else
        btnText = tr("Get Detail");
    
    // return btnText;
    // return need update
    if(!wtx.Gettlementtxid().size() && wtx.Getorderstatus() == "1")
        return true;
    
    return false;
}

void DockerOrderView::chooseDate(int idx)
{
    if(!dockerorderProxyModel)
        return;
    
    QSettings settings;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        dockerorderProxyModel->setDateRange(
                DockerOrderFilterProxy::MIN_DATE,
                DockerOrderFilterProxy::MAX_DATE);
        break;
    case Today:
        dockerorderProxyModel->setDateRange(
                QDateTime(current),
                DockerOrderFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        dockerorderProxyModel->setDateRange(
                QDateTime(startOfWeek),
                DockerOrderFilterProxy::MAX_DATE);

        } break;
    case ThisMonth:
        dockerorderProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                DockerOrderFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        dockerorderProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        dockerorderProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                DockerOrderFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
    // Persist new date settings
    settings.setValue("transactionDate", idx);
    if (dateWidget->itemData(idx).toInt() == Range){
        settings.setValue("transactionDateFrom", dateFrom->date().toString(PERSISTENCE_DATE_FORMAT));
        settings.setValue("transactionDateTo", dateTo->date().toString(PERSISTENCE_DATE_FORMAT));
    }
}

void DockerOrderView::chooseType(int idx)
{
    if(!dockerorderProxyModel)
        return;
    dockerorderProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
    // Persist settings
    QSettings settings;
    settings.setValue("transactionType", idx);
}

void DockerOrderView::chooseWatchonly(int idx)
{
    if(!dockerorderProxyModel)
        return;
    dockerorderProxyModel->setWatchOnlyFilter(
        (DockerOrderFilterProxy::WatchOnlyFilter)watchOnlyWidget->itemData(idx).toInt());
}

void DockerOrderView::changedPrefix(const QString &prefix)
{
    if(!dockerorderProxyModel)
        return;
    dockerorderProxyModel->setAddressPrefix(prefix);
}

void DockerOrderView::txidPrefix(const QString &prefix)
{
    if(!dockerorderProxyModel)
        return;
        
    int rowCount = dockerorderView->model()->rowCount();

    for(int i=0;i<rowCount;i++){
        QString txid = dockerorderView->model()->index(i,0).data(DockerOrderTableModel::TxIDRole).toString();

        if(!txid.contains(prefix, Qt::CaseInsensitive)){
            dockerorderView->hideRow(i);
        }
        else{
            dockerorderView->showRow(i);
        }

    }    
}

void DockerOrderView::changedAmount(const QString &amount)
{
    if(!dockerorderProxyModel)
        return;
    CAmount amount_parsed = 0;

    // Replace "," by "." so MassGridUnits::parse will not fail for users entering "," as decimal separator
    QString newAmount = amount;
    newAmount.replace(QString(","), QString("."));

    if(MassGridUnits::parse(model->getOptionsModel()->getDisplayUnit(), newAmount, &amount_parsed))
    {
        dockerorderProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        dockerorderProxyModel->setMinAmount(0);
    }
}

void DockerOrderView::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Transaction History"), QString(),
        tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(dockerorderProxyModel);
    writer.addColumn(tr("Confirmed"), 0, DockerOrderTableModel::ConfirmedRole);
    if (model && model->haveWatchOnly())
        writer.addColumn(tr("Watch-only"), DockerOrderTableModel::Watchonly);
    writer.addColumn(tr("Date"), 0, DockerOrderTableModel::DateRole);
    writer.addColumn(tr("TxID"), DockerOrderTableModel::TxID, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, DockerOrderTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, DockerOrderTableModel::AddressRole);
    writer.addColumn(MassGridUnits::getAmountColumnTitle(model->getOptionsModel()->getDisplayUnit()), 0, DockerOrderTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, DockerOrderTableModel::TxIDRole);

    if(!writer.write()) {
        Q_EMIT message(tr("Exporting Failed"), tr("There was an error trying to save the transaction history to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Exporting Successful"), tr("The transaction history was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void DockerOrderView::contextualMenu(const QPoint &point)
{
    QModelIndex index = dockerorderView->indexAt(point);
    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    // check if transaction can be abandoned, disable context menu action in case it doesn't
    uint256 hash;
    hash.SetHex(selection.at(0).data(DockerOrderTableModel::TxHashRole).toString().toStdString());
    abandonAction->setEnabled(model->transactionCanBeAbandoned(hash));

    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void DockerOrderView::abandonTx()
{
    if(!dockerorderView || !dockerorderView->selectionModel())
        return;
    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows(0);

    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(DockerOrderTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());

    // Abandon the wallet transaction over the walletModel
    model->abandonTransaction(hash);

    // Update the table
    model->getDockerOrderTableModel()->updateTransaction(hashQStr, CT_UPDATED, false);
}

void DockerOrderView::copyAddress()
{
    GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::AddressRole);
}

void DockerOrderView::copyLabel()
{
    GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::LabelRole);
}

void DockerOrderView::copyAmount()
{
    GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::FormattedAmountRole);
}

void DockerOrderView::copyTxID()
{
    GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::TxIDRole);
}

void DockerOrderView::copyTxHex()
{
    GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::TxHexRole);
}

void DockerOrderView::copyTxPlainText()
{
    GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::TxPlainTextRole);
}

void DockerOrderView::editLabel()
{
    if(!dockerorderView->selectionModel() ||!model)
        return;
    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(DockerOrderTableModel::AddressRole).toString();
        if(address.isEmpty())
        {
            // If this transaction has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a transaction is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(AddressTableModel::TypeRole).toString();

            EditAddressDialog dlg(
                type == AddressTableModel::Receive
                ? EditAddressDialog::EditReceivingAddress
                : EditAddressDialog::EditSendingAddress, this);
            dlg.setModel(addressBook);
            dlg.loadRow(idx);
            QPoint pos = MassGridGUI::winPos();
            QSize size = MassGridGUI::winSize();
            dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
            dlg.exec();
        }
        else
        {
            // Add sending address
            EditAddressDialog dlg(EditAddressDialog::NewSendingAddress,
                this);
            dlg.setModel(addressBook);
            dlg.setAddress(address);
            QPoint pos = MassGridGUI::winPos();
            QSize size = MassGridGUI::winSize();
            dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
            dlg.exec();
        }
    }
}

void DockerOrderView::showOrderDetail(CWalletTx& wtx)
{
    DockerOrderDescDialog dlg(model);
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();

    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
    dlg.setwalletTx(wtx);
    dlg.exec();
}

void DockerOrderView::deleteService()
{
    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows();
    if(selection.isEmpty())
        return ;
    int count = selection.size();

    for(int i=0;i<count;i++){
        QModelIndex index = selection.at(i);

        QString txidStr = dockerorderView->model()->index(index.row(),DockerOrderTableModel::TxID).data().toString();
        CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr.toStdString())];  //watch only not check
        std::string orderstatusStr = wtx.Getorderstatus();
        std::string masternodeip = wtx.Getmasternodeip();
        // std::string serviceidStr = wtx.Getserviceid();
        if(orderstatusStr != "1"){
            Q_EMIT deleteService(txidStr.split("-").at(0).toStdString(),masternodeip);
            break;
        }
    }
}

void DockerOrderView::getCurrentItemTxidAndmnIp(std::string &txid,std::string &masternodeip,std::string &orderstatusStr)
{
    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows();
    if(selection.isEmpty())
        return ;
    int count = selection.size();

    for(int i=0;i<count;i++){
        QModelIndex index = selection.at(i);
        txid = dockerorderView->model()->index(index.row(),DockerOrderTableModel::TxID).data().toString().toStdString();
        CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txid)];  //watch only not check
        orderstatusStr = wtx.Getorderstatus();
        masternodeip = wtx.Getmasternodeip();
        // std::string serviceidStr = wtx.Getserviceid();
        break;
    }
}

/** Compute sum of all selected transactions */
void DockerOrderView::computeSum()
{
    qint64 amount = 0;
    int nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    if(!dockerorderView->selectionModel())
        return;

    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows();

    Q_FOREACH (QModelIndex index, selection){
        amount += index.data(DockerOrderTableModel::AmountRole).toLongLong();
    }

    QString strAmount(MassGridUnits::formatWithUnit(nDisplayUnit, amount, true, MassGridUnits::separatorAlways));
    if (amount < 0) strAmount = "<span style='color:red;'>" + strAmount + "</span>";
    Q_EMIT trxAmount(strAmount);

}

void DockerOrderView::openThirdPartyTxUrl(QString url)
{
    if(!dockerorderView || !dockerorderView->selectionModel())
        return;
    QModelIndexList selection = dockerorderView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(DockerOrderTableModel::TxHashRole).toString())));
}

QWidget *DockerOrderView::createDateRangeWidget()
{
    // Create default dates in case nothing is persisted
    QString defaultDateFrom = QDate::currentDate().toString(PERSISTENCE_DATE_FORMAT);
    QString defaultDateTo = QDate::currentDate().addDays(1).toString(PERSISTENCE_DATE_FORMAT);
    QSettings settings;
 
    dateRangeWidget = new QFrame();
    dateRangeWidget->setObjectName("dateRangeWidget");
    dateRangeWidget->setStyleSheet("QFrame#dateRangeWidget{border:hidden;background-color: rgb(255, 255, 255);}");
    dateRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateRangeWidget->setContentsMargins(1,1,1,1);
    dateRangeWidget->setMinimumHeight(35);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new MDateEdit(this); //MDateEdit
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    // Load persisted FROM date
    dateFrom->setDate(QDate::fromString(settings.value("transactionDateFrom", defaultDateFrom).toString(), PERSISTENCE_DATE_FORMAT));

    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new MDateEdit(this); //QDateEdit
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    // Load persisted TO date
    dateTo->setDate(QDate::fromString(settings.value("transactionDateTo", defaultDateTo).toString(), PERSISTENCE_DATE_FORMAT));

    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    connect(dateTo, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));

    return dateRangeWidget;
}

void DockerOrderView::dateRangeChanged()
{
    if(!dockerorderProxyModel)
        return;
    
    // Persist new date range
    QSettings settings;
    settings.setValue("transactionDateFrom", dateFrom->date().toString(PERSISTENCE_DATE_FORMAT));
    settings.setValue("transactionDateTo", dateTo->date().toString(PERSISTENCE_DATE_FORMAT));
    
    dockerorderProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()));
}

void DockerOrderView::focusTransaction(const QModelIndex &idx)
{
    if(!dockerorderProxyModel)
        return;
    QModelIndex targetIdx = dockerorderProxyModel->mapFromSource(idx);
    dockerorderView->selectRow(targetIdx.row());
    // computeSum();
    dockerorderView->scrollTo(targetIdx);
    dockerorderView->setCurrentIndex(targetIdx);
    dockerorderView->setFocus();
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void DockerOrderView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(DockerOrderTableModel::ToAddress);
}

// Need to override default Ctrl+C action for amount as default behaviour is just to copy DisplayRole text
bool DockerOrderView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
             GUIUtil::copyEntryData(dockerorderView, 0, DockerOrderTableModel::TxPlainTextRole);
             return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// show/hide column Watch-only
void DockerOrderView::updateWatchOnlyColumn(bool fHaveWatchOnly)
{
    watchOnlyWidget->setVisible(true);
    dockerorderView->setColumnHidden(DockerOrderTableModel::Watchonly, !fHaveWatchOnly);
}

void DockerOrderView::updateTransData(QString txid)
{
    if(m_syncTransactionThread == NULL){
        m_syncTransactionThread = new SyncTransactionHistoryThread(0);
        connect(m_syncTransactionThread,SIGNAL(syncTaskEnd(const QString&,bool)),this,SLOT(updateTransactionHistoryData(const QString&,bool)));
        m_syncTransactionThread->start();
    }
    m_syncTransactionThread->addTask(txid);
}

void DockerOrderView::updateTransactionHistoryData(const QString& txid,bool sucess)
{
    LogPrintf("====>DockerOrderView::updateTransactionHistoryData txid:%s\n",txid.toStdString());
    LogPrintf("====>DockerOrderView::updateTransactionHistoryData sucess:%d\n",sucess);
}

SyncTransactionHistoryThread::SyncTransactionHistoryThread(QObject* parent):
    QThread(parent)
{

}

SyncTransactionHistoryThread::~SyncTransactionHistoryThread()
{

}

void SyncTransactionHistoryThread::addTask(const QString& txid)
{
    if(!m_taskList.count(txid))
        m_taskList.append(txid);
    m_wait.wakeOne();
}

void SyncTransactionHistoryThread::setNeedWork(bool type)
{
    m_isNeedWork = type;
}

bool SyncTransactionHistoryThread::isNeedWork()
{
    return m_isNeedWork;
}

bool SyncTransactionHistoryThread::popTask(QString& txid)
{
    if(m_taskList.size() > 0){
        txid = m_taskList.at(0);
        m_taskList.removeAt(0);
        return true;
    }
    return false;
}

bool SyncTransactionHistoryThread::doTask(const QString& txid)
{
    std::string txidStr = txid.toStdString();
    // std::string ip_port = "118.25.224.128:19443";
    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txidStr)];  //watch only not chec
    std::string ip_port = wtx.Getmasternodeip();

    if(!dockercluster.SetConnectDockerAddress(ip_port) || !dockercluster.ProcessDockernodeConnections()){
        // CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!"));
        LogPrintf("SyncTransactionHistoryThread connect to docker failed!\n");
        return false;
    }
    dockerServerman.setTRANSDataStatus(CDockerServerman::AskTD);
    int updateCountMsec = 0;
    dockercluster.AskForTransData(txidStr);
    while(true){
        CDockerServerman::TRANSDATASTATUS status = dockerServerman.getTRANSDataStatus();
        if(status == CDockerServerman::ReceivedTD)
            return true;
        QThread::sleep(1);
        LogPrintf("=====>SyncTransactionHistoryThread::doTask ask TransData\n");
        //check sync time out
        if((++updateCountMsec) >= SYNCTRANSTINEOUT)
            break;
    }
    return false;
}

void SyncTransactionHistoryThread::run()
{
    QString txid;
    while(isNeedWork()){
        QMutexLocker locker(&m_mutex);
        if(popTask(txid)){
            bool sucess = doTask(txid);
            Q_EMIT syncTaskEnd(txid,sucess);
        }
        else{
            m_wait.wait(&m_mutex);
        }
    }
    dockerServerman.setTRANSDataStatus(CDockerServerman::FreeTD);

    Q_EMIT syncThreadFinished();
}
