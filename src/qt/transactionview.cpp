// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionview.h"

#include "addresstablemodel.h"
#include "massgridunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactiondescdialog.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "massgridgui.h"
#include "ui_interface.h"
// #include "mitemdelegate.h"
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
static const char* PERSISTENCE_DATE_FORMAT = "yyyy-MM-dd";

TransactionView::TransactionView(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent), model(0), transactionProxyModel(0),
    transactionView(0), abandonAction(0), columnResizingFixer(0)
{
    QSettings settings;
    // Build filter row
    setContentsMargins(0,0,0,0);

    transactionView = 0;
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
    watchOnlyWidget->addItem("", TransactionFilterProxy::WatchOnlyFilter_All);
    watchOnlyWidget->addItem(QIcon(":/icons/eye_plus"), "", TransactionFilterProxy::WatchOnlyFilter_Yes);
    watchOnlyWidget->addItem(QIcon(":/icons/eye_minus"), "", TransactionFilterProxy::WatchOnlyFilter_No);
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

    transactionView = view;

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

    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(copyTxHexAction);
    contextMenu->addAction(copyTxPlainText);
    contextMenu->addAction(showDetailsAction);
    contextMenu->addSeparator();
    contextMenu->addAction(abandonAction);
    contextMenu->addAction(editLabelAction);

    mapperThirdPartyTxUrls = new QSignalMapper(this);

    // Connect actions
    connect(mapperThirdPartyTxUrls, SIGNAL(mapped(QString)), this, SLOT(openThirdPartyTxUrl(QString)));

    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(watchOnlyWidget, SIGNAL(activated(int)), this, SLOT(chooseWatchonly(int)));
    // connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    // connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));

    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
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
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
}

void TransactionView::setSearchWidget(QComboBox* dateComboBox,QComboBox* typeComboBox,QLineEdit* addrLineEdit)
{
    dateWidget = dateComboBox;
    typeWidget = typeComboBox;
    // addressWidget = addrLineEdit;

    // ad->dl
       //QComboBox
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

    typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                                        TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                                  TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    typeWidget->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));



#if QT_VERSION >= 0x040700
    addrLineEdit->setPlaceholderText(tr("Enter address or label to search"));
#endif
}
void TransactionView::setModel(WalletModel *model)
{
    QSettings settings;
    this->model = model;
    if(model)
    {
        transactionProxyModel = new TransactionFilterProxy(this);
        transactionProxyModel->setSourceModel(model->getTransactionTableModel());
        transactionProxyModel->setDynamicSortFilter(true);
        transactionProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        transactionProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        transactionProxyModel->setSortRole(Qt::EditRole);

        transactionView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        transactionView->setModel(transactionProxyModel);
        transactionView->setAlternatingRowColors(true);
        transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
        transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transactionView->setSortingEnabled(true);
        transactionView->sortByColumn(TransactionTableModel::Status, Qt::DescendingOrder);
        transactionView->verticalHeader()->hide();

        transactionView->setColumnWidth(TransactionTableModel::Status, STATUS_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Watchonly, WATCHONLY_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Date, DATE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Type, TYPE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);
        transactionView->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        // Note: it's a good idea to connect this signal AFTER the model is set
        // connect(transactionView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(computeSum()));
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(transactionView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this);

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
        
        // Update transaction list with persisted settings
        chooseType(settings.value("transactionType").toInt());
        // chooseDate(settings.value("transactionDate").toInt());
    }
}

void TransactionView::chooseDate(int idx)
{
    if(!transactionProxyModel)
        return;
    
    QSettings settings;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        transactionProxyModel->setDateRange(
                TransactionFilterProxy::MIN_DATE,
                TransactionFilterProxy::MAX_DATE);
        break;
    case Today:
        transactionProxyModel->setDateRange(
                QDateTime(current),
                TransactionFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        transactionProxyModel->setDateRange(
                QDateTime(startOfWeek),
                TransactionFilterProxy::MAX_DATE);

        } break;
    case ThisMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                TransactionFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                TransactionFilterProxy::MAX_DATE);
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

void TransactionView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
    // Persist settings
    QSettings settings;
    settings.setValue("transactionType", idx);
}

void TransactionView::chooseWatchonly(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setWatchOnlyFilter(
        (TransactionFilterProxy::WatchOnlyFilter)watchOnlyWidget->itemData(idx).toInt());
}

void TransactionView::changedPrefix(const QString &prefix)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setAddressPrefix(prefix);
}

void TransactionView::changedAmount(const QString &amount)
{
    if(!transactionProxyModel)
        return;
    CAmount amount_parsed = 0;

    // Replace "," by "." so MassGridUnits::parse will not fail for users entering "," as decimal separator
    QString newAmount = amount;
    newAmount.replace(QString(","), QString("."));

    if(MassGridUnits::parse(model->getOptionsModel()->getDisplayUnit(), newAmount, &amount_parsed))
    {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}

void TransactionView::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Transaction History"), QString(),
        tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(transactionProxyModel);
    writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
    if (model && model->haveWatchOnly())
        writer.addColumn(tr("Watch-only"), TransactionTableModel::Watchonly);
    writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
    writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
    writer.addColumn(MassGridUnits::getAmountColumnTitle(model->getOptionsModel()->getDisplayUnit()), 0, TransactionTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, TransactionTableModel::TxIDRole);

    if(!writer.write()) {
        Q_EMIT message(tr("Exporting Failed"), tr("There was an error trying to save the transaction history to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Exporting Successful"), tr("The transaction history was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void TransactionView::contextualMenu(const QPoint &point)
{
    QModelIndex index = transactionView->indexAt(point);
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    // check if transaction can be abandoned, disable context menu action in case it doesn't
    uint256 hash;
    hash.SetHex(selection.at(0).data(TransactionTableModel::TxHashRole).toString().toStdString());
    abandonAction->setEnabled(model->transactionCanBeAbandoned(hash));

    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TransactionView::abandonTx()
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);

    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(TransactionTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());

    // Abandon the wallet transaction over the walletModel
    model->abandonTransaction(hash);

    // Update the table
    model->getTransactionTableModel()->updateTransaction(hashQStr, CT_UPDATED, false);
}

void TransactionView::copyAddress()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void TransactionView::copyLabel()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void TransactionView::copyAmount()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

void TransactionView::copyTxID()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxIDRole);
}

void TransactionView::copyTxHex()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxHexRole);
}

void TransactionView::copyTxPlainText()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
}

void TransactionView::editLabel()
{
    if(!transactionView->selectionModel() ||!model)
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(TransactionTableModel::AddressRole).toString();
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

void TransactionView::showDetails()
{
    if(!transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog dlg(selection.at(0));
        QPoint pos = MassGridGUI::winPos();
        QSize size = MassGridGUI::winSize();

        dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
        dlg.exec();
    }
}

/** Compute sum of all selected transactions */
void TransactionView::computeSum()
{
    qint64 amount = 0;
    int nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    if(!transactionView->selectionModel())
        return;

    QModelIndexList selection = transactionView->selectionModel()->selectedRows();

    Q_FOREACH (QModelIndex index, selection){
        amount += index.data(TransactionTableModel::AmountRole).toLongLong();
    }

    QString strAmount(MassGridUnits::formatWithUnit(nDisplayUnit, amount, true, MassGridUnits::separatorAlways));
    if (amount < 0) strAmount = "<span style='color:red;'>" + strAmount + "</span>";
    Q_EMIT trxAmount(strAmount);

}

void TransactionView::openThirdPartyTxUrl(QString url)
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(TransactionTableModel::TxHashRole).toString())));
}

QWidget *TransactionView::createDateRangeWidget()
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

void TransactionView::dateRangeChanged()
{
    if(!transactionProxyModel)
        return;
    
    // Persist new date range
    QSettings settings;
    settings.setValue("transactionDateFrom", dateFrom->date().toString(PERSISTENCE_DATE_FORMAT));
    settings.setValue("transactionDateTo", dateTo->date().toString(PERSISTENCE_DATE_FORMAT));
    
    transactionProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()));
}

void TransactionView::focusTransaction(const QModelIndex &idx)
{
    if(!transactionProxyModel)
        return;
    QModelIndex targetIdx = transactionProxyModel->mapFromSource(idx);
    transactionView->selectRow(targetIdx.row());
    // computeSum();
    transactionView->scrollTo(targetIdx);
    transactionView->setCurrentIndex(targetIdx);
    transactionView->setFocus();
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void TransactionView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(TransactionTableModel::ToAddress);
}

// Need to override default Ctrl+C action for amount as default behaviour is just to copy DisplayRole text
bool TransactionView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
             GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
             return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// show/hide column Watch-only
void TransactionView::updateWatchOnlyColumn(bool fHaveWatchOnly)
{
    watchOnlyWidget->setVisible(true);
    transactionView->setColumnHidden(TransactionTableModel::Watchonly, !fHaveWatchOnly);
}
MDateEdit::MDateEdit(QWidget *parent)
    : QDateEdit(parent)
{
    this->setCalendarPopup(true);
    m_DefCalendar = new DefineCalendar(this);
    // m_DefCalendar->setLocale(QLocale(QLocale::English));

    setCalendarWidget(m_DefCalendar);

    m_DefCalendar->disconnect(SIGNAL(clicked(QDate)));
    connect(m_DefCalendar, SIGNAL(setFinished(QDateTime)), this, SLOT(getDateTime(QDateTime)));

    setMyStytle();

}

MDateEdit::~MDateEdit()
{

}

void MDateEdit::getDateTime( const QDateTime &dateTime )
{
    this->setDateTime(dateTime);
}

void MDateEdit::setMyStytle()
{
    QString strTemp;
    //QWidget
    strTemp.append("QWidget{font:normal 10pt Microsoft YaHei;}");
    strTemp.append("QWidget#CalTopWidget{background-color:rgb(172,99,43);}"); 
    strTemp.append("QWidget#CalBottomWidget{background-color:white;}");
    //QLabel
    strTemp.append("QLabel#CalLabel{border:1px solid lightgray; color:rgb(172,99,43);}");

    //QPushButton
    strTemp.append("QPushButton#CalPushBtnT1{border:0px;}");
    strTemp.append("QPushButton#CalPushBtnT1:hover,QPushButton#CalPushBtnT1:pressed{background-color:rgb(239,169,4);}");
                   strTemp.append("QPushButton#CalPushBtnT2{border:1px solid lightgray; color:rgb(172,99,43);}");
            strTemp.append("QPushButton#CalPushBtnT2:hover,QPushButton#CalPushBtnT2:pressed{background-color:rgb(231, 231, 231);}");
   //QComboBox
// ad->al
   strTemp.append("QComboBox#CalComboBox{border:0px; background-color:rgb(172,99,43); \
                    color:white; height:24px; width:40px;}\
                    QComboBox#CalComboBox::down-arrow{\
                    border:hidden;\
                    background-color:rgb(172,99,43);\
                    border-image:url(:/res/pic/ad.png);}\
                    QComboBox#CalComboBox::drop-down{width:14px; border:0px;}\
                    QComboBox#CalComboBox QAbstractItemView {\
                    color:rgb(172,99,43);\
                    border: 1px solid rgb(172,99,43);\
                    background-color:white;\
                    selection-color:white;\
                    selection-background-color: rgb(239,169,4);}\
                    QComboBox#CalComboBox QAbstractItemView::item{\
                        height: 30px;\
                        background-color: rgb(198, 125, 26);\
                        border:hidden;\
                        color: rgb(255, 255, 255);\
                    }");
    //QTimeEdit
   // au->ar
    strTemp.append("QTimeEdit#CalTimeEdit{ border:1px solid lightgray; color:rgb(172,99,43);}");
    strTemp.append("QTimeEdit#CalTimeEdit:!enabled{ background:rgb(65, 65, 65); color:rgb(90, 90, 90); border:0px; }");
    strTemp.append("QTimeEdit#CalTimeEdit::up-button{  background:rgb(172,99,43);width: 16px;  border-width: 1px;}");
    strTemp.append("QTimeEdit#CalTimeEdit::up-button:hover{ background:rgb(239,169,4); }");
    strTemp.append("QTimeEdit#CalTimeEdit::up-button:!enabled{ background:rgb(65, 65, 65); }");
    strTemp.append("QTimeEdit#CalTimeEdit::up-arrow{  border-image:url(:/res/pic/au.png);}");

    strTemp.append("QTimeEdit#CalTimeEdit::down-button{  background:rgb(172,99,43); width: 16px;  border-width: 1px;}");
    strTemp.append("QTimeEdit#CalTimeEdit::down-button:hover{ background:rgb(239,169,4); }");
    strTemp.append("QTimeEdit#CalTimeEdit::down-button:!enabled{ background:rgb(65, 65, 65); }");
    strTemp.append("QTimeEdit#CalTimeEdit::down-arrow{  \
                    border:hidden;\
                    background-color:rgb(172,99,43);\
                    border-image:url(:/res/pic/ad.png);}");
// ad->al
    //QDateEdit
    strTemp.append("QDateEdit{border:1px solid rgb(172,99,43); height:24px; }");
    strTemp.append("QDateEdit::down-arrow{border-image:url(:/res/pic/calendar.png); height:15px; width:20px;}");
    strTemp.append("QDateEdit::drop-down{width:30px; border:0px solid red;\
                   subcontrol-origin: padding;subcontrol-position: top right;}");

    //QScrollBar
    strTemp.append("QScrollBar:vertical{background-color:white; width: 10px;}\
                   QScrollBar::handle:vertical{background-color:rgb(172,99,43); border:1px solid white;border-radius:2px; min-height:8px}\
                   QScrollBar::handle:vertical:hover{background-color:rgb(239,169,4);}\
                   QScrollBar::sub-line{background-color:white;}\
                   QScrollBar::add-line{background-color:white;}");

    this->setStyleSheet(strTemp);
}


DefineCalendar::DefineCalendar(QWidget *parent)
    : QCalendarWidget(parent)
{
    setMinimumDate(QDate(2000,1,1));
    setMaximumDate(QDate(2100,1,1));
    InitWidgets();
    // setMinimumWidth(460);
    setMinimumHeight(240);
    setNavigationBarVisible(false);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    // this->layout()->setSizeConstraint(QLayout::SetFixedSize);
    connect(this, SIGNAL(currentPageChanged(int,int)), this, SLOT(CurPageChange(int,int)));
    UpdateYear();

}

DefineCalendar::~DefineCalendar()
{

}

void DefineCalendar::paintCell( QPainter *painter, const QRect &rect, const QDate &date ) const
{
    if (date == this->selectedDate())
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(172,99,43));
        painter->drawRect(rect);
        painter->setPen(Qt::white);
        painter->drawText(rect,Qt::AlignCenter,QString::number(date.day()));
        painter->restore();
    }
    else
    {
        QCalendarWidget::paintCell(painter,rect,date);
    }

}

void DefineCalendar::InitWidgets()
{
    //top
    widget_top = new QWidget(this);
    comboBox_Year = new QComboBox(this);
    comboBox_Month = new QComboBox(this);
    pushBtn_YL = new QPushButton(this);
    pushBtn_YR = new QPushButton(this);
    pushBtn_ML = new QPushButton(this);
    pushBtn_MR = new QPushButton(this);

    // QListView *yearListview = new QListView(); 
    // QListView *monthListview = new QListView(); 
    // yearListview->setMaximumHeight(300);
    // monthListview->setMaximumHeight(300);
    comboBox_Year->setView(new QListView());
    comboBox_Month->setView(new QListView());

    QStringList monthList;
    monthList<<tr("Jan")<<tr("Feb")<<tr("Mar")<<tr("Apr")<<tr("May")<<tr("Jun")
            <<tr("Jul")<<tr("Aug")<<tr("Sep")<<tr("Oct")<<tr("Nov")<<tr("Dec");
    comboBox_Month->addItems(monthList);

    int nO = 24;
    int nI = 20;
    pushBtn_YL->setFixedSize(nO,nO);
    pushBtn_YL->setIconSize(QSize(nI,nI));
    pushBtn_YL->setIcon(QPixmap(":/res/pic/al.png"));

    pushBtn_YR->setFixedSize(nO,nO);
    pushBtn_YR->setIconSize(QSize(nI,nI));
    pushBtn_YR->setIcon(QPixmap(":/res/pic/ar.png"));

    pushBtn_ML->setFixedSize(nO,nO);
    pushBtn_ML->setIconSize(QSize(nI,nI));
    pushBtn_ML->setIcon(QPixmap(":/res/pic/al.png"));

    pushBtn_MR->setFixedSize(nO,nO);
    pushBtn_MR->setIconSize(QSize(nI,nI));
    pushBtn_MR->setIcon(QPixmap(":/res/pic/ar.png"));

    QHBoxLayout *HTopLayout = new QHBoxLayout;
    HTopLayout->setContentsMargins(4,4,4,4);
    HTopLayout->setSpacing(0);
    widget_top->setLayout(HTopLayout);
    HTopLayout->addWidget(pushBtn_YL);
    HTopLayout->addWidget(comboBox_Year);
    HTopLayout->addWidget(pushBtn_YR);
    HTopLayout->addStretch(1);
    HTopLayout->addWidget(pushBtn_ML);
    HTopLayout->addWidget(comboBox_Month);
    HTopLayout->addWidget(pushBtn_MR);

    // HTopLayout->setSizeConstraint(QLayout::SetFixedSize);


    QVBoxLayout *VMainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    VMainLayout->insertWidget(0,widget_top);

    widget_top->setObjectName("CalTopWidget");
    comboBox_Year->setObjectName("CalComboBox");
    comboBox_Month->setObjectName("CalComboBox");
    pushBtn_YL->setObjectName("CalPushBtnT1");
    pushBtn_YR->setObjectName("CalPushBtnT1");
    pushBtn_ML->setObjectName("CalPushBtnT1");
    pushBtn_MR->setObjectName("CalPushBtnT1");

#if defined(Q_OS_MAC)
    comboBox_Year->setStyle(QStyleFactory::create("Windows"));
    comboBox_Month->setStyle(QStyleFactory::create("Windows"));
#endif

    connect(pushBtn_YL, SIGNAL(clicked()), this, SLOT(BtnSlots()));
    connect(pushBtn_YR, SIGNAL(clicked()), this, SLOT(BtnSlots()));
    connect(pushBtn_ML, SIGNAL(clicked()), this, SLOT(BtnSlots()));
    connect(pushBtn_MR, SIGNAL(clicked()), this, SLOT(BtnSlots()));

    connect(comboBox_Year, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboBoxSlots(int)));
    connect(comboBox_Month, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboBoxSlots(int)));
}

void DefineCalendar::UpdateYear()
{
    comboBox_Year->clear();
    QDate d1 = this->minimumDate();
    QDate d2 = this->maximumDate();
    for (int i = d1.year(); i<= d2.year(); i++)
    {
        comboBox_Year->addItem(tr("%1").arg(i));
    }
}

void DefineCalendar::SetToday()
{
    QDate curDate = QDate::currentDate();
    int year = curDate.year();
    int month = curDate.month();
    this->setSelectedDate(curDate);
    QString yearStr = QString::number(year);
    comboBox_Year->setCurrentText(yearStr);
    comboBox_Month->setCurrentIndex(month-1);
}

void DefineCalendar::ClearTime()
{

}

void DefineCalendar::BtnSlots()
{
    QPushButton *pBtn = qobject_cast<QPushButton *>(sender());

    if (pBtn == pushBtn_YL)
    {
        int curInt = comboBox_Year->currentIndex()-1;
        if (curInt<=0)
        {
            curInt = 0;
        }
        comboBox_Year->setCurrentIndex(curInt);
    }
    else if (pBtn == pushBtn_YR)
    {
        int curInt = comboBox_Year->currentIndex()+1;
        if (curInt > comboBox_Year->count()-1)
        {
            curInt = comboBox_Year->count()-1;
        }
        comboBox_Year->setCurrentIndex(curInt);
    }
    else if (pBtn == pushBtn_ML)
    {
        int curInt = comboBox_Month->currentIndex()-1;
        if (curInt<=0)
        {
            curInt = 0;
        }
        comboBox_Month->setCurrentIndex(curInt);
    }
    else if (pBtn == pushBtn_MR)
    {
        int curInt = comboBox_Month->currentIndex()+1;
        if (curInt > comboBox_Month->count()-1)
        {
            curInt = comboBox_Month->count()-1;
        }
        comboBox_Month->setCurrentIndex(curInt);
    }
    UpdatePage();
}

void DefineCalendar::ComboBoxSlots( int index )
{
    UpdatePage();
}

void DefineCalendar::UpdatePage()
{
    int nYear = comboBox_Year->currentText().toInt();
    int nMonth = comboBox_Month->currentIndex()+1;
    this->setCurrentPage(nYear,nMonth);
}

void DefineCalendar::CurPageChange( int year, int month )
{
    comboBox_Year->setCurrentText(QString::number(year));
    comboBox_Month->setCurrentIndex(month-1);

    QDate curDate;
    curDate.setDate(year,month,1);
    Q_EMIT setFinished(QDateTime(curDate));
}


MItemDelegate::MItemDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{

}

MItemDelegate::~MItemDelegate()
{

}

void MItemDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //去掉Focus
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    if (option.state.testFlag(QStyle::State_HasFocus))
    {
        viewOption.state = viewOption.state ^ QStyle::State_HasFocus;
    }
    QStyledItemDelegate::paint(painter, viewOption, index);
}
