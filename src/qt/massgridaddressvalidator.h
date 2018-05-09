// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_MASSGRIDADDRESSVALIDATOR_H
#define MASSGRID_QT_MASSGRIDADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class MassGridAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MassGridAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** MassGrid address widget validator, checks for a valid massgrid address.
 */
class MassGridAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MassGridAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // MASSGRID_QT_MASSGRIDADDRESSVALIDATOR_H
