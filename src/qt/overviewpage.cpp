// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "massgridunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "utilitydialog.h"
#include "walletmodel.h"

#include "instantx.h"
#include "masternode-sync.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define ICON_OFFSET 16
#define DECORATION_SIZE 54
#define NUM_ITEMS 5
#define NUM_ITEMS_ADV 7
#define ITEM_SPACING 10

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(MassGridUnits::MGD),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QFont font = painter->font();
        font.setPixelSize(16);
        // font.setBold(true);
        painter->setFont(font);

        QRect mainRect = option.rect;
        // static int index_2 =0;
        if(index.row() %2){
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(247,242,238)/*QColor(247,242,238)*/);
            painter->drawRect(mainRect);
        }
        else{
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::white);
            painter->drawRect(mainRect);
        }

        QRect iconRect = QRect(mainRect.x(),mainRect.y(),mainRect.width()*0.2,mainRect.height());

        QRect dateRect = QRect(iconRect.width()+iconRect.x(),mainRect.y(),mainRect.width()*0.3,mainRect.height());

        QRect _amountRect = QRect(dateRect.width()+dateRect.x(),mainRect.y(),mainRect.width()*0.5-20,mainRect.height());

        QRect timeRect = QRect(dateRect.x(),dateRect.y(),dateRect.width(),dateRect.height()/2-ITEM_SPACING/2);
        QRect stateRect = QRect(dateRect.x(),dateRect.y()+dateRect.height()-(dateRect.height()/2-ITEM_SPACING/2),
                                dateRect.width(),dateRect.height()/2-ITEM_SPACING/2);

        QRect amountRect = QRect(_amountRect.x(),_amountRect.y(),_amountRect.width(),_amountRect.height()/2-ITEM_SPACING/2);
        QRect addressRect = QRect(_amountRect.x(),_amountRect.y()+_amountRect.height()-(_amountRect.height()/2-ITEM_SPACING/2),
                           _amountRect.width(),_amountRect.height()/2-ITEM_SPACING/2);


        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

        QRect decorationRect(QPoint(20,mainRect.topLeft().y()), QSize(DECORATION_SIZE, DECORATION_SIZE)); //mainRect.topLeft()
        // int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        // QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        // QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignRight|Qt::AlignTop, address, &boundingRect); //Qt::AlignLeft|Qt::AlignVCenter

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }


        QString transactionsType = tr("Received with");

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
            transactionsType = tr("Send to");
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {

            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = MassGridUnits::formatWithUnit(unit, amount, true, MassGridUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignBottom, amountText); //Qt::AlignRight|Qt::AlignVCenter

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(timeRect, Qt::AlignLeft|Qt::AlignBottom, GUIUtil::dateTimeStr(date)); //Qt::AlignLeft|Qt::AlignVCenter

        painter->drawText(stateRect, Qt::AlignLeft|Qt::AlignTop, transactionsType); //Qt::AlignLeft|Qt::AlignVCenter

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;
};
#include "overviewpage.moc"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate(platformStyle, this)),
    timer(nullptr)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    // Note: minimum height of listTransactions will be set later in updateAdvancedPSUI() to reflect actual settings
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");


    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    ui->stackedWidget->setCurrentIndex(0);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
/*
    ui->labelBalance->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, balance, false, MassGridUnits::separatorAlways));
    ui->labelUnconfirmed->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, MassGridUnits::separatorAlways));
    ui->labelImmature->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, MassGridUnits::separatorAlways));
    ui->labelTotal->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, balance + unconfirmedBalance + immatureBalance, false, MassGridUnits::separatorAlways));
*/
    QString Balance = MassGridUnits::formatWithUnit(nDisplayUnit, balance, false, MassGridUnits::separatorAlways);
    QString Unconfirmed = MassGridUnits::formatWithUnit(nDisplayUnit, unconfirmedBalance, false, MassGridUnits::separatorAlways);
    QString Immature = MassGridUnits::formatWithUnit(nDisplayUnit, immatureBalance, false, MassGridUnits::separatorAlways);
    QString Total = MassGridUnits::formatWithUnit(nDisplayUnit, balance + unconfirmedBalance + immatureBalance, false, MassGridUnits::separatorAlways);


    ui->labelBalance->setText(Balance);
    ui->labelUnconfirmed->setText(Unconfirmed);
    ui->labelImmature->setText(MassGridUnits::formatWithUnit(nDisplayUnit, immatureBalance, false, MassGridUnits::separatorAlways));
    ui->labelTotal->setText(Total);
    ui->labelWatchAvailable->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance, false, MassGridUnits::separatorAlways));
    ui->labelWatchPending->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, MassGridUnits::separatorAlways));
    ui->labelWatchImmature->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, MassGridUnits::separatorAlways));
    ui->labelWatchTotal->setText(MassGridUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, MassGridUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    static int cachedTxLocks = 0;

    if(cachedTxLocks != nCompleteTXLocks){
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
	Q_EMIT updateBalance(Balance,Unconfirmed,Immature,showImmature,showWatchOnlyImmature,Total);

}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly){
        ui->labelWatchImmature->hide();
    }
    else{
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // update the display unit, to not use the default ("MGD")
        updateDisplayUnit();
        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(advancedPSUIChanged(bool)), this, SLOT(updateAdvancedPSUI(bool)));
        // explicitly update PS frame and transaction list to reflect actual settings
        updateAdvancedPSUI(model->getOptionsModel()->getShowAdvancedPSUI());

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateAdvancedPSUI(bool fShowAdvancedPSUI) {
    this->fShowAdvancedPSUI = fShowAdvancedPSUI;
    int nNumItems = (fLiteMode || !fShowAdvancedPSUI) ? NUM_ITEMS : NUM_ITEMS_ADV;
    SetupTransactionList(nNumItems);

    if (fLiteMode) return;
}

void OverviewPage::SetupTransactionList(int nNumItems) {
    ui->listTransactions->setMinimumHeight(nNumItems * (DECORATION_SIZE + 2));

    if(walletModel && walletModel->getOptionsModel()) {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(walletModel->getTransactionTableModel());
        filter->setLimit(nNumItems);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);
    }
}
