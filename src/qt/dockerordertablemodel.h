// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_DOCKERORDERTABLEMODEL_H
#define MASSGRID_QT_DOCKERORDERTABLEMODEL_H

#include "massgridunits.h"

#include <QAbstractTableModel>
#include <QStringList>

class PlatformStyle;
class DockerOrderRecord;
class DockerOrderTablePriv;
class WalletModel;
class CWalletTx;

class CWallet;

/** UI model for the transaction table of a wallet.
 */
class DockerOrderTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DockerOrderTableModel(const PlatformStyle *platformStyle, CWallet* wallet, WalletModel *parent = 0);
    ~DockerOrderTableModel();

    enum ColumnIndex {
        Status = 0,
        Watchonly = 1,
        Date = 2,
        TxID = 3,
        ToAddress = 4,
        Amount = 5,
        OrderStatus = 6,
        Operation = 7
    };

    /** Roles to get specific information from a transaction row.
        These are independent of column.
    */
    enum RoleIndex {
        /** Type of transaction */
        TypeRole = Qt::UserRole,
        /** Date and time this transaction was created */
        DateRole,
        /** Watch-only boolean */
        WatchonlyRole,
        /** Watch-only icon */
        WatchonlyDecorationRole,
        /** Long description (HTML format) */
        LongDescriptionRole,
        /** Address of transaction */
        AddressRole,
        /** Label of address related to transaction */
        LabelRole,
        /** Net amount of transaction */
        AmountRole,
        /** Unique identifier */
        TxIDRole,
        /** Transaction hash */
        TxHashRole,
        /** Transaction data, hex-encoded */
        TxHexRole,
        /** Whole transaction as plain text */
        TxPlainTextRole,
        /** Is transaction confirmed? */
        ConfirmedRole,
        /** Formatted amount, without brackets when unconfirmed */
        FormattedAmountRole,
        /** Transaction status (DockerOrderRecord::Status) */
        StatusRole,
        /** Unprocessed icon */
        RawDecorationRole,
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    bool processingQueuedTransactions() { return fProcessingQueuedTransactions; }
    void refreshModel();
    std::list<std::string> getRerentTxidList();
    void getTransactionDetail(CWalletTx wtx,QString& date,CAmount& amount);

private:
    CWallet* wallet;
    WalletModel *walletModel;
    QStringList columns;
    DockerOrderTablePriv *priv;
    bool fProcessingQueuedTransactions;
    const PlatformStyle *platformStyle;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant addressColor(const DockerOrderRecord *wtx) const;
    QString formatTxStatus(const DockerOrderRecord *wtx) const;
    QString formatTxDate(const DockerOrderRecord *wtx) const;
    QString formatTxType(const DockerOrderRecord *wtx) const;
    QString formatTxToAddress(const DockerOrderRecord *wtx, bool tooltip) const;
    QString formatTxAmount(const DockerOrderRecord *wtx, bool showUnconfirmed=true, MassGridUnits::SeparatorStyle separators=MassGridUnits::separatorStandard) const;
    QString formatTooltip(const DockerOrderRecord *rec) const;
    QString fotmatOperation(const DockerOrderRecord *rec,const QModelIndex &index) const;
    QVariant txStatusDecoration(const DockerOrderRecord *wtx) const;
    QVariant txWatchonlyDecoration(const DockerOrderRecord *wtx) const;
    QVariant txAddressDecoration(const DockerOrderRecord *wtx) const;
    QString formatOrderStatus(const DockerOrderRecord *wtx,const QModelIndex &index) const;
    bool checkIsSpent(const DockerOrderRecord *rec,int vout_n) const;

public Q_SLOTS:
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString &hash, int status, bool showTransaction);
    void updateConfirmations();
    void updateDisplayUnit();
    /** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
    void updateAmountColumnTitle();
    /* Needed to update fProcessingQueuedTransactions through a QueuedConnection */
    void setProcessingQueuedTransactions(bool value) { fProcessingQueuedTransactions = value; }

    friend class DockerOrderTablePriv;

Q_SIGNALS:
    void updateOrderStatus(const std::string&)const;
    void addOperationBtn(int row)const;
    void deleteTransaction(int index)const;
};

#endif // MASSGRID_QT_DOCKERORDERTABLEMODEL_H
