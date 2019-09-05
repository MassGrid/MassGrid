// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dockerordertablemodel.h"
// #include 
#include "addresstablemodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "dockerorderdesc.h"
#include "dockerorderrecord.h"
#include "walletmodel.h"

#include "core_io.h"
#include "validation.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "wallet/wallet.h"
#include "txmempool.h"
#include "validation.h"
#include "masternodeman.h"

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QPushButton>

#include <boost/foreach.hpp>

// Amount column is right-aligned it contains numbers
static int column_alignments2[] = {
        Qt::AlignLeft|Qt::AlignVCenter, /* status */
        Qt::AlignLeft|Qt::AlignVCenter, /* watchonly */
        Qt::AlignLeft|Qt::AlignVCenter, /* date */
        Qt::AlignLeft|Qt::AlignVCenter, /* type */
        Qt::AlignLeft|Qt::AlignVCenter, /* address */
        Qt::AlignRight|Qt::AlignVCenter, /* amount */
        Qt::AlignHCenter|Qt::AlignVCenter, /* amount */
        Qt::AlignHCenter|Qt::AlignVCenter /* amount */
    };

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const DockerOrderRecord &a, const DockerOrderRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const DockerOrderRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const DockerOrderRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class DockerOrderTablePriv
{
public:
    DockerOrderTablePriv(CWallet *wallet, DockerOrderTableModel *parent) :
        wallet(wallet),
        parent(parent)
    {
    }

    CWallet *wallet;
    DockerOrderTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<DockerOrderRecord> cachedWallet;
    std::list<std::string> reletTxids;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
        qDebug() << "DockerOrderTablePriv::refreshWallet";
        cachedWallet.clear();
        reletTxids.clear();
        {
            LOCK2(cs_main, wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                if(DockerOrderRecord::showTransaction(it->second) && it->second.Getmasternodeoutpoint().size()){
                    if(!it->second.Getmasternodeip().size()){
                        getMasternodeInfo(it->second);
                    }
                    if(it->second.GetCreateOutPoint().size()){
                        reletTxids.push_back(it->first.ToString());
                    }
                    cachedWallet.append(DockerOrderRecord::decomposeTransaction(wallet, it->second));
                }
            }
        }
    }

    void getMasternodeInfo(CWalletTx &wtx)
    {
        std::map<COutPoint, CMasternode> mapMasternodes = mnodeman.GetFullMasternodeMap();

        QString masternodeoutpointStr = QString::fromStdString(wtx.Getmasternodeoutpoint());
        std::string hashStr = masternodeoutpointStr.split("-").at(0).toStdString();
        QString hashStr_Q = QString::fromStdString(hashStr);
        int n = masternodeoutpointStr.split("-").at(1).toInt();
        CMasternode node = mapMasternodes[COutPoint(uint256S(hashStr),(uint32_t)n)];

        //find masternode address
        BOOST_FOREACH(const CTxOut& txout, wtx.vout) {
            if(txout.scriptPubKey.Find(OP_RETURN)){
                // txNew.vout[n].scriptPubKey = scriptMsg;
                QString scriptPubKeyStr =  QString::fromStdString(ScriptToAsmStr(txout.scriptPubKey)).split(" ").at(1);
                QString indexN = scriptPubKeyStr.mid(8,8);
                bool ok;
                int n = indexN.toInt(&ok,16);
                txnouttype type;
                vector<CTxDestination> addresses;
                int nRequired;
                if (!ExtractDestinations(wtx.vout[n].scriptPubKey, type, addresses, nRequired)) {
                    // out.push_back(Pair("type", GetTxnOutputType(type)));
                    return;
                }
                BOOST_FOREACH(const CTxDestination& addr, addresses)
                    wtx.Setmasternodeaddress(CMassGridAddress(addr).ToString());
                break;
            }
        }

        std::string strFilter = "";
        for (auto& mnpair : mapMasternodes) {
            // CMasternode mn = mnpair.second;
            std::string strOutpoint = mnpair.first.ToStringShort();
            std::string masternodeip = mnpair.second.addr.ToString();
            if(wtx.Getmasternodeoutpoint() != strOutpoint)
                continue;

            wtx.Setmasternodeip(masternodeip);
            wtx.Setorderstatus("0");
            CWalletDB walletdb(wallet->strWalletFile);
            wtx.WriteToDisk(&walletdb);         
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status, bool showTransaction)
    {        
        LOCK2(cs_main, wallet->cs_wallet);
        qDebug() << "DockerOrderTablePriv::updateWallet: " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);

        // Find bounds of this transaction in model
        QList<DockerOrderRecord>::iterator lower = qLowerBound(
            cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        QList<DockerOrderRecord>::iterator upper = qUpperBound(
            cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        int lowerIndex = (lower - cachedWallet.begin());
        int upperIndex = (upper - cachedWallet.begin());
        bool inModel = (lower != upper);

        if(status == CT_UPDATED)
        {
            if(showTransaction && !inModel)
                status = CT_NEW; /* Not in model, but want to show, treat as new */
            if(!showTransaction && inModel)
                status = CT_DELETED; /* In model, but want to hide, treat as deleted */
        }

        qDebug() << "    inModel=" + QString::number(inModel) +
                    " Index=" + QString::number(lowerIndex) + "-" + QString::number(upperIndex) +
                    " showTransaction=" + QString::number(showTransaction) + " derivedStatus=" + QString::number(status);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "DockerOrderTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is already in model";
                break;
            }
            if(showTransaction)
            {
                LOCK2(cs_main, wallet->cs_wallet);
                // Find transaction in wallet
                std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
                if(mi == wallet->mapWallet.end())
                {
                    qWarning() << "DockerOrderTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is not in wallet";
                    break;
                }
                // Added -- insert at the right position
                if(!mi->second.Getmasternodeoutpoint().size()){
                    return ;
                }

                // if(mi->second.GetCreateOutPoint().size() > 0){
                //     return ;
                // }
                
                QList<DockerOrderRecord> toInsert =
                        DockerOrderRecord::decomposeTransaction(wallet, mi->second);
                if(!toInsert.isEmpty()) /* only if something to insert */
                {
                    parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                    int insert_idx = lowerIndex;
                    Q_FOREACH(const DockerOrderRecord &rec, toInsert)
                    {
                        cachedWallet.insert(insert_idx, rec);
                        insert_idx += 1;
                    }
                    parent->endInsertRows();
                    Q_EMIT parent->addOperationBtn(0);
                }
            }
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "DockerOrderTablePriv::updateWallet: Warning: Got CT_DELETED, but transaction is not in model";
                break;
            }
            // Removed -- remove entire transaction from table
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedWallet.erase(lower, upper);
            parent->endRemoveRows();
            Q_EMIT parent->deleteTransaction(lowerIndex);
            break;
        case CT_UPDATED:
            // Miscellaneous updates -- nothing to do, status update will take care of this, and is only computed for
            // visible transactions.
            break;
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    DockerOrderRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            DockerOrderRecord *rec = &cachedWallet[idx];

            // Get required locks upfront. This avoids the GUI from getting
            // stuck if the core is holding the locks for a longer time - for
            // example, during a wallet rescan.
            //
            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            TRY_LOCK(cs_main, lockMain);
            if(lockMain)
            {
                TRY_LOCK(wallet->cs_wallet, lockWallet);
                if(lockWallet && rec->statusUpdateNeeded())
                {
                    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);

                    if(mi != wallet->mapWallet.end())
                    {
                        rec->updateStatus(mi->second);
                    }
                }
            }
            return rec;
        }
        return 0;
    }

    QString describe(DockerOrderRecord *rec, int unit)
    {
        {
            LOCK2(cs_main, wallet->cs_wallet);
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
            if(mi != wallet->mapWallet.end())
            {
                return DockerOrderDesc::toHTML(wallet, mi->second, rec, unit);
            }
        }
        return QString();
    }

    QString getTxHex(DockerOrderRecord *rec)
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
        if(mi != wallet->mapWallet.end())
        {
            std::string strHex = EncodeHexTx(static_cast<CTransaction>(mi->second));
            return QString::fromStdString(strHex);
        }
        return QString();
    }
    
    CTransaction getTc(DockerOrderRecord *rec)
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
        if(mi != wallet->mapWallet.end())
        {
            return static_cast<CTransaction>(mi->second);
        }
        return CTransaction();
    }
};

DockerOrderTableModel::DockerOrderTableModel(const PlatformStyle *platformStyle, CWallet* wallet, WalletModel *parent):
        QAbstractTableModel(parent),
        wallet(wallet),
        walletModel(parent),
        priv(new DockerOrderTablePriv(wallet, this)),
        fProcessingQueuedTransactions(false),
        platformStyle(platformStyle)
{
    columns << QString() << QString() << tr("Date") << tr("TxID") << tr("Address / Label") 
    << MassGridUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit())
    << tr("OrderStatus") << tr("Operate");
    priv->refreshWallet();

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    subscribeToCoreSignals();
}

DockerOrderTableModel::~DockerOrderTableModel()
{
    unsubscribeFromCoreSignals();
    delete priv;
}

/** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
void DockerOrderTableModel::updateAmountColumnTitle()
{
    columns[Amount] = MassGridUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit());
    Q_EMIT headerDataChanged(Qt::Horizontal,Amount,Amount);
}

void DockerOrderTableModel::updateTransaction(const QString &hash, int status, bool showTransaction)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(updated, status, showTransaction);
}

void DockerOrderTableModel::updateConfirmations()
{
    // Blocks came in since last poll.
    // Invalidate status (number of confirmations) and (possibly) description
    //  for all rows. Qt is smart enough to only actually request the data for the
    //  visible rows.
    Q_EMIT dataChanged(index(0, Status), index(priv->size()-1, Status));
    Q_EMIT dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
}

int DockerOrderTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int DockerOrderTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QString DockerOrderTableModel::formatTxStatus(const DockerOrderRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case DockerOrderStatus::OpenUntilBlock:
        status = tr("Open for %n more block(s)","",wtx->status.open_for);
        break;
    case DockerOrderStatus::OpenUntilDate:
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));
        break;
    case DockerOrderStatus::Offline:
        status = tr("Offline");
        break;
    case DockerOrderStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case DockerOrderStatus::Abandoned:
        status = tr("Abandoned");
        break;
    case DockerOrderStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(DockerOrderRecord::RecommendedNumConfirmations);
        break;
    case DockerOrderStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    case DockerOrderStatus::Conflicted:
        status = tr("Conflicted");
        break;
    case DockerOrderStatus::Immature:
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case DockerOrderStatus::MaturesWarning:
        status = tr("This block was not received by any other nodes and will probably not be accepted!");
        break;
    case DockerOrderStatus::NotAccepted:
        status = tr("Generated but not accepted");
        break;
    }

    return status;
}

QString DockerOrderTableModel::formatTxDate(const DockerOrderRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    return QString();
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString DockerOrderTableModel::lookupAddress(const std::string &address, bool tooltip) const
{
    QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    QString description;
    if(!label.isEmpty())
    {
        description += label;
    }
    if(label.isEmpty() || tooltip)
    {
        description += QString(" (") + QString::fromStdString(address) + QString(")");
    }
    return description;
}

QString DockerOrderTableModel::formatTxType(const DockerOrderRecord *wtx) const
{
    switch(wtx->type)
    {
    case DockerOrderRecord::RecvWithAddress:
        return tr("Received with");
    case DockerOrderRecord::RecvFromOther:
        return tr("Received from");
    case DockerOrderRecord::RecvWithPrivateSend:
        return tr("Received via PrivateSend");
    case DockerOrderRecord::SendToAddress:
    case DockerOrderRecord::SendToOther:
        return tr("Sent to");
    case DockerOrderRecord::SendToSelf:
        return tr("Payment to yourself");
    case DockerOrderRecord::Generated:
        return tr("Mined");

    case DockerOrderRecord::PrivateSendDenominate:
        return tr("PrivateSend Denominate");
    case DockerOrderRecord::PrivateSendCollateralPayment:
        return tr("PrivateSend Collateral Payment");
    case DockerOrderRecord::PrivateSendMakeCollaterals:
        return tr("PrivateSend Make Collateral Inputs");
    case DockerOrderRecord::PrivateSendCreateDenominations:
        return tr("PrivateSend Create Denominations");
    case DockerOrderRecord::PrivateSend:
        return tr("PrivateSend");

    default:
        return QString();
    }
}

QVariant DockerOrderTableModel::txAddressDecoration(const DockerOrderRecord *wtx) const
{
    switch(wtx->type)
    {
    case DockerOrderRecord::Generated:
        return QIcon(":/icons/res/icons/tx_mined");
    case DockerOrderRecord::RecvWithPrivateSend:
    case DockerOrderRecord::RecvWithAddress:
    case DockerOrderRecord::RecvFromOther:
        return QIcon(":/icons/res/icons/tx_input");
    case DockerOrderRecord::SendToAddress:
    case DockerOrderRecord::SendToOther:
        return QIcon(":/icons/res/icons/tx_output");
    default:
        return QIcon(":/icons/res/icons/tx_inout");
    }
}

QString DockerOrderTableModel::formatTxToAddress(const DockerOrderRecord *wtx, bool tooltip) const
{
    QString watchAddress;
    if (tooltip) {
        // Mark transactions involving watch-only addresses by adding " (watch-only)"
        watchAddress = wtx->involvesWatchAddress ? QString(" (") + tr("watch-only") + QString(")") : "";
    }

    switch(wtx->type)
    {
    case DockerOrderRecord::RecvFromOther:
        return QString::fromStdString(wtx->address) + watchAddress;
    case DockerOrderRecord::RecvWithAddress:
    case DockerOrderRecord::RecvWithPrivateSend:
    case DockerOrderRecord::SendToAddress:
    case DockerOrderRecord::Generated:
    case DockerOrderRecord::PrivateSend:
        return lookupAddress(wtx->address, tooltip) + watchAddress;
    case DockerOrderRecord::SendToOther:
        return QString::fromStdString(wtx->address) + watchAddress;
    case DockerOrderRecord::SendToSelf:
    default:
        return tr("(n/a)") + watchAddress;
    }
}

QVariant DockerOrderTableModel::addressColor(const DockerOrderRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case DockerOrderRecord::RecvWithAddress:
    case DockerOrderRecord::SendToAddress:
    case DockerOrderRecord::Generated:
    case DockerOrderRecord::PrivateSend:
    case DockerOrderRecord::RecvWithPrivateSend:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
        } break;
    case DockerOrderRecord::SendToSelf:
    case DockerOrderRecord::PrivateSendCreateDenominations:
    case DockerOrderRecord::PrivateSendDenominate:
    case DockerOrderRecord::PrivateSendMakeCollaterals:
    case DockerOrderRecord::PrivateSendCollateralPayment:
        return COLOR_BAREADDRESS;
    default:
        break;
    }
    return QVariant();
}

QString DockerOrderTableModel::formatTxAmount(const DockerOrderRecord *wtx, bool showUnconfirmed, MassGridUnits::SeparatorStyle separators) const
{
    QString str = MassGridUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit + wtx->debit, false, separators);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}

QVariant DockerOrderTableModel::txStatusDecoration(const DockerOrderRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case DockerOrderStatus::OpenUntilBlock:
    case DockerOrderStatus::OpenUntilDate:
        return COLOR_TX_STATUS_OPENUNTILDATE;
    case DockerOrderStatus::Offline:
        return COLOR_TX_STATUS_OFFLINE;
    case DockerOrderStatus::Unconfirmed:
        return QIcon(":/icons/transaction_0");
    case DockerOrderStatus::Abandoned:
        return QIcon(":/icons/transaction_abandoned");
    case DockerOrderStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case DockerOrderStatus::Confirmed:
        return QIcon(":/icons/transaction_confirmed");
    case DockerOrderStatus::Conflicted:
        return QIcon(":/icons/transaction_conflicted");
    case DockerOrderStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case DockerOrderStatus::MaturesWarning:
    case DockerOrderStatus::NotAccepted:
        return QIcon(":/icons/transaction_0");
    default:
        return COLOR_BLACK;
    }
}

QVariant DockerOrderTableModel::txWatchonlyDecoration(const DockerOrderRecord *wtx) const
{
    if (wtx->involvesWatchAddress)
        return QIcon(":/icons/eye");
    else
        return QVariant();
}

QString DockerOrderTableModel::formatTooltip(const DockerOrderRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==DockerOrderRecord::RecvFromOther || rec->type==DockerOrderRecord::SendToOther ||
       rec->type==DockerOrderRecord::SendToAddress || rec->type==DockerOrderRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QString DockerOrderTableModel::formatOrderStatus(const DockerOrderRecord *rec,const QModelIndex &index) const
{
#ifdef ENABLE_WALLET
    LOCK2(cs_main, wallet->cs_wallet);
#else
    LOCK(cs_main);
#endif

    CWalletTx& wtx = wallet->mapWallet[rec->hash];
    if(wtx.Getstate() == "completed" ){
        return QString("Settled");
    }
    else{
        return QString("Paid");
    }

    // CTransaction tx;
    // uint256 hashBlock;
    // CWalletTx& wtx = wallet->mapWallet[rec->hash];  //watch only not check
    // if (!GetTransaction(rec->hash, tx, Params().GetConsensus(), hashBlock, true)){
    //     LogPrintf("No information available about transaction\n");
    //         return QString("NULL"); 
    // }

    // for (unsigned int i = 0; i < tx.vout.size(); i++) {
    //     const CTxOut& txout = tx.vout[i];

    //     txnouttype type;
    //     std::vector<CTxDestination> addresses;
    //     int nRequired;

    //     if (!ExtractDestinations(txout.scriptPubKey, type, addresses, nRequired)) {
    //         // LogPrintf("Can't find vout address type:%d,nRequired:%d,txout n:%d\n",type,nRequired,i);
    //         continue;
    //     }

    //     BOOST_FOREACH(const CTxDestination& addr, addresses){
    //         if(CMassGridAddress(addr).ToString() == wtx.Getmasternodeaddress()){

    //             bool isSpent = checkIsSpent(rec,i); 
    //             std::string orderStatusStr = isSpent?"1":"0";  

    //             if(wtx.Getorderstatus() != orderStatusStr){
    //                 wtx.Setorderstatus(orderStatusStr);
    //                 CWalletDB walletdb(wallet->strWalletFile);
    //                 wtx.WriteToDisk(&walletdb);
    //                 Q_EMIT updateOrderStatus(rec->getTxID().toStdString());
    //             }
    //             return isSpent ? tr("Settled") : tr("Paid");
    //         }
    //     }
    // }
    // LogPrintf("Can't find vout address the order status is null\n");
    // return QString("NULL"); 
}

QVariant DockerOrderTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    DockerOrderRecord *rec = static_cast<DockerOrderRecord*>(index.internalPointer());

    switch(role)
    {
    case RawDecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case Watchonly:
            return txWatchonlyDecoration(rec);
        case ToAddress:
            return txAddressDecoration(rec);
        }
        break;
    case Qt::DecorationRole:
    {
        return qvariant_cast<QIcon>(index.data(RawDecorationRole));
    }
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case TxID:
            // return formatTxType(rec);
            return rec->getTxID();
        case ToAddress:
            return formatTxToAddress(rec, false);
        case Amount:
            return formatTxAmount(rec, true, MassGridUnits::separatorAlways);
        case OrderStatus:
            return formatOrderStatus(rec,index) ;
        case Operation:
            return tr("");
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case TxID:
            // return formatTxType(rec);
            return rec->getTxID();
        case Watchonly:
            return (rec->involvesWatchAddress ? 1 : 0);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Amount:
            return formatOrderStatus(rec,index);
            // return qint64(rec->credit + rec->debit);
        case Operation:
            return tr("");
        }
        break;
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments2[index.column()];
    case Qt::ForegroundRole:
        // Use the "danger" color for abandoned transactions
        if(rec->status.status == DockerOrderStatus::Abandoned)
        {
            return COLOR_TX_STATUS_DANGER;
        }
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != DockerOrderStatus::Immature)
        {
            return COLOR_UNCONFIRMED;
        }
        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
        {
            return COLOR_NEGATIVE;
        }
        if(index.column() == ToAddress)
        {
            return addressColor(rec);
        }
        break;
    case TypeRole:
        return rec->type;
    case DateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
    case WatchonlyRole:
        return rec->involvesWatchAddress;
    case WatchonlyDecorationRole:
        return txWatchonlyDecoration(rec);
    case LongDescriptionRole:
        return priv->describe(rec, walletModel->getOptionsModel()->getDisplayUnit());
    case AddressRole:
        return QString::fromStdString(rec->address);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case AmountRole:
        return qint64(rec->credit + rec->debit);
    case TxIDRole:
        return rec->getTxID();
    case TxHashRole:
        return QString::fromStdString(rec->hash.ToString());
    case TxHexRole:
        return priv->getTxHex(rec);
    case TxPlainTextRole:
        {
            QString details;
            QString txLabel = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));

            details.append(formatTxDate(rec));
            details.append(" ");
            details.append(formatTxStatus(rec));
            details.append(". ");
            if(!formatTxType(rec).isEmpty()) {
                details.append(formatTxType(rec));
                details.append(" ");
            }
            if(!rec->address.empty()) {
                if(txLabel.isEmpty())
                    details.append(tr("(no label)") + " ");
                else {
                    details.append("(");
                    details.append(txLabel);
                    details.append(") ");
                }
                details.append(QString::fromStdString(rec->address));
                details.append(" ");
            }
            details.append(formatTxAmount(rec, false, MassGridUnits::separatorNever));
            return details;
        }
    case ConfirmedRole:
        return rec->status.countsForBalance;
    case FormattedAmountRole:
        // Used for copy/export, so don't include separators
        return formatTxAmount(rec, false, MassGridUnits::separatorNever);
    case StatusRole:
        return rec->status.status;
    }
    return QVariant();
}

QVariant DockerOrderTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return column_alignments2[section];
        } else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Status:
                return tr("Transaction status. Hover over this field to show number of confirmations.");
            case Date:
                return tr("Date and time that the transaction was received.");
            case TxID:
                return tr("TxID of transaction.");
            case Watchonly:
                return tr("Whether or not a watch-only address is involved in this transaction.");
            case ToAddress:
                return tr("User-defined intent/purpose of the transaction.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            }
        }
    }
    return QVariant();
}

QModelIndex DockerOrderTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    DockerOrderRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

void DockerOrderTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Amount column with the current unit
    updateAmountColumnTitle();
    Q_EMIT dataChanged(index(0, Amount), index(priv->size()-1, Amount));
}

// // queue notifications to show a non freezing progress dialog e.g. for rescan
// struct TransactionNotification
// {
// public:
//     TransactionNotification() {}
//     TransactionNotification(uint256 hash, ChangeType status, bool showTransaction):
//         hash(hash), status(status), showTransaction(showTransaction) {}

//     void invoke(QObject *ttm)
//     {
//         QString strHash = QString::fromStdString(hash.GetHex());
//         qDebug() << "NotifyTransactionChanged: " + strHash + " status= " + QString::number(status);
//         QMetaObject::invokeMethod(ttm, "updateTransaction", Qt::QueuedConnection,
//                                   Q_ARG(QString, strHash),
//                                   Q_ARG(int, status),
//                                   Q_ARG(bool, showTransaction));
//     }
// private:
//     uint256 hash;
//     ChangeType status;
//     bool showTransaction;
// };

// static bool fQueueNotifications_DockerOrder = false;
// static std::vector< TransactionNotification > vQueueNotifications;

// static void NotifyTransactionChanged(DockerOrderTableModel *ttm, CWallet *wallet, const uint256 &hash, ChangeType status)
// {
//     // Find transaction in wallet
//     std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
//     // Determine whether to show transaction or not (determine this here so that no relocking is needed in GUI thread)
//     bool inWallet = mi != wallet->mapWallet.end();
//     bool showTransaction = (inWallet && DockerOrderRecord::showTransaction(mi->second));

//     TransactionNotification notification(hash, status, showTransaction);

//     if (fQueueNotifications)
//     {
//         vQueueNotifications.push_back(notification);
//         return;
//     }
//     notification.invoke(ttm);
// }

// static void ShowProgress(DockerOrderTableModel *ttm, const std::string &title, int nProgress)
// {
//     if (nProgress == 0)
//         fQueueNotifications = true;

//     if (nProgress == 100)
//     {
//         fQueueNotifications = false;
//         if (vQueueNotifications.size() > 10) // prevent balloon spam, show maximum 10 balloons
//             QMetaObject::invokeMethod(ttm, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, true));
//         for (unsigned int i = 0; i < vQueueNotifications.size(); ++i)
//         {
//             if (vQueueNotifications.size() - i <= 10)
//                 QMetaObject::invokeMethod(ttm, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, false));

//             vQueueNotifications[i].invoke(ttm);
//         }
//         std::vector<TransactionNotification >().swap(vQueueNotifications); // clear
//     }
// }

void DockerOrderTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    // wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    // wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void DockerOrderTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    // wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    // wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}

bool DockerOrderTableModel::checkIsSpent(const DockerOrderRecord *rec,int vout_n) const
{

#ifdef ENABLE_WALLET
    LOCK2(cs_main, wallet->cs_wallet);
#else
    LOCK(cs_main);
#endif
    // RPCTypeCheck(params, boost::assign::list_of(UniValue::VSTR)(UniValue::VARR)(UniValue::VARR)(UniValue::VSTR), true);

    CTransaction tx = priv->getTc(const_cast<DockerOrderRecord *>(rec));

    // throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transaction");

    COutPoint outpoint = COutPoint(tx.GetHash(),vout_n);
    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view
        view.AccessCoin(outpoint); // Load entries from viewChain into view; can fail.
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    const Coin& coin = view.AccessCoin(outpoint); //txin.prevout
    if (coin.IsSpent()) {
        // LogPrintf("Input not found or already spent\n");
        return true;
    }

    return false;
}

void DockerOrderTableModel::refreshModel()
{ 
    priv->refreshWallet();
}

std::list<std::string> DockerOrderTableModel::getRerentTxidList()
{
    refreshModel();
    return priv->reletTxids;
}
    static QList<DockerOrderRecord> decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx);

void DockerOrderTableModel::getTransactionDetail(CWalletTx wtx,QString& date,CAmount& amount)
{
    QList<DockerOrderRecord> records = DockerOrderRecord::decomposeTransaction(wallet, wtx);
    if(records.size()){
        date = formatTxDate(&records[0]);
        amount = records[0].debit;
        // amount = formatTxAmount(&records[0],false);

        LogPrintf("======>getTransactionDetail records[0].credit:%ld\n",records[0].credit);
        LogPrintf("======>getTransactionDetail records[0].debit:%ld\n",records[0].debit);
    }
}

