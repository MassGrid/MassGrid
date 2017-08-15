// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MLGBCOIN_QT_MLGBCOINADDRESSVALIDATOR_H
#define MLGBCOIN_QT_MLGBCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class MLGBcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MLGBcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** MLGBcoin address widget validator, checks for a valid mlgbcoin address.
 */
class MLGBcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MLGBcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // MLGBCOIN_QT_MLGBCOINADDRESSVALIDATOR_H
