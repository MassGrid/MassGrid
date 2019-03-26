// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_DOCKERORDERDESC_H
#define MASSGRID_QT_DOCKERORDERDESC_H

#include <QObject>
#include <QString>

class DockerOrderRecord;

class CWallet;
class CWalletTx;

/** Provide a human-readable extended HTML description of a transaction.
 */
class DockerOrderDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(CWallet *wallet, CWalletTx &wtx, DockerOrderRecord *rec, int unit);

private:
    DockerOrderDesc() {}

    static QString FormatTxStatus(const CWalletTx& wtx);
};

#endif // MASSGRID_QT_DOCKERORDERDESC_H
