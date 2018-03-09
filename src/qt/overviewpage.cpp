// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "massgridunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3
#define ITEM_SPACING 10

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(MassGridUnits::MGD)
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

        static int index_2 =0;
        if(index_2++ %2){
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

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
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
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    ui->stackedWidget->setCurrentIndex(1);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}                   

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    QString Balance = MassGridUnits::formatWithUnit(unit, balance, false, MassGridUnits::separatorAlways);
    QString Unconfirmed = MassGridUnits::formatWithUnit(unit, unconfirmedBalance, false, MassGridUnits::separatorAlways);
    QString Total = MassGridUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, MassGridUnits::separatorAlways);

    ui->labelBalance->setText(Balance);
    ui->labelUnconfirmed->setText(Unconfirmed);
    ui->labelImmature->setText(MassGridUnits::formatWithUnit(unit, immatureBalance, false, MassGridUnits::separatorAlways));
    ui->labelTotal->setText(Total);
    ui->labelWatchAvailable->setText(MassGridUnits::formatWithUnit(unit, watchOnlyBalance, false, MassGridUnits::separatorAlways));
    ui->labelWatchPending->setText(MassGridUnits::formatWithUnit(unit, watchUnconfBalance, false, MassGridUnits::separatorAlways));
    ui->labelWatchImmature->setText(MassGridUnits::formatWithUnit(unit, watchImmatureBalance, false, MassGridUnits::separatorAlways));
    ui->labelWatchTotal->setText(MassGridUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, MassGridUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    emit updateBalance(Balance,Unconfirmed,Total);
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

    if (!showWatchOnly)
        ui->labelWatchImmature->hide();
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
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("MGD")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

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
