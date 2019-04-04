// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dockerorderfilterproxy.h"

#include "dockerordertablemodel.h"
#include "dockerorderrecord.h"

#include <cstdlib>

#include <QDateTime>

// Earliest date that can be represented (far in the past)
const QDateTime DockerOrderFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime DockerOrderFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

DockerOrderFilterProxy::DockerOrderFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    dateFrom(MIN_DATE),
    dateTo(MAX_DATE),
    addrPrefix(),
    txidPrefix(),
    typeFilter(COMMON_TYPES),
    watchOnlyFilter(WatchOnlyFilter_All),
    minAmount(0),
    limitRows(-1),
    showInactive(true)
{
}

bool DockerOrderFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int type = index.data(DockerOrderTableModel::TypeRole).toInt();
    QDateTime datetime = index.data(DockerOrderTableModel::DateRole).toDateTime();
    bool involvesWatchAddress = index.data(DockerOrderTableModel::WatchonlyRole).toBool();
    QString address = index.data(DockerOrderTableModel::AddressRole).toString();
    QString label = index.data(DockerOrderTableModel::LabelRole).toString();
    qint64 amount = llabs(index.data(DockerOrderTableModel::AmountRole).toLongLong());
    int status = index.data(DockerOrderTableModel::StatusRole).toInt();
    QString txid = index.data(DockerOrderTableModel::TxIDRole).toString();

    if(!showInactive && status == DockerOrderStatus::Conflicted)
        return false;
    if(!(TYPE(type) & typeFilter))
        return false;
    if (involvesWatchAddress && watchOnlyFilter == WatchOnlyFilter_No)
        return false;
    if (!involvesWatchAddress && watchOnlyFilter == WatchOnlyFilter_Yes)
        return false;
    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if (!address.contains(addrPrefix, Qt::CaseInsensitive) && !label.contains(addrPrefix, Qt::CaseInsensitive))
        return false;
    if(amount < minAmount)
        return false;
    if(!txid.contains(txidPrefix, Qt::CaseInsensitive)){
        return false;
    }

    return true;
}

void DockerOrderFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
}

void DockerOrderFilterProxy::setAddressPrefix(const QString &addrPrefix)
{
    this->addrPrefix = addrPrefix;
    invalidateFilter();
}

void DockerOrderFilterProxy::setTxidPrefix(const QString &txidPrefix)
{
    this->txidPrefix = txidPrefix;
    invalidateFilter();
}

void DockerOrderFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void DockerOrderFilterProxy::setMinAmount(const CAmount& minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void DockerOrderFilterProxy::setWatchOnlyFilter(WatchOnlyFilter filter)
{
    this->watchOnlyFilter = filter;
    invalidateFilter();
}

void DockerOrderFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

void DockerOrderFilterProxy::setShowInactive(bool showInactive)
{
    this->showInactive = showInactive;
    invalidateFilter();
}

int DockerOrderFilterProxy::rowCount(const QModelIndex &parent) const
{
    if(limitRows != -1)
    {
        return std::min(QSortFilterProxyModel::rowCount(parent), limitRows);
    }
    else
    {
        return QSortFilterProxyModel::rowCount(parent);
    }
}
