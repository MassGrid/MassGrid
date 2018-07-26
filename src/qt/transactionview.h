// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_TRANSACTIONVIEW_H
#define MASSGRID_QT_TRANSACTIONVIEW_H

#include "guiutil.h"

#include <QWidget>
#include <QKeyEvent>

#include <QDateTimeEdit>
#include <QDateTime>
#include <QDateEdit>
#include <QCalendarWidget>

#include <QObject>
#include <qstyleditemdelegate.h>

class PlatformStyle;
class TransactionFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QItemSelectionModel;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;
class MDateEdit;


class QLabel;
class QComboBox;
class QPushButton;
class QTimeEdit;
class QDateTime;
QT_END_NAMESPACE

/** Widget showing the transaction list for a wallet, including a filter row.
    Using the filter row, the user can view or export a subset of the transactions.
  */
class TransactionView : public QWidget
{
    Q_OBJECT

public:
    explicit TransactionView(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *model);

    // Date ranges for filter
    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        STATUS_COLUMN_WIDTH = 30,
        WATCHONLY_COLUMN_WIDTH = 23,
        DATE_COLUMN_WIDTH = 120,
        TYPE_COLUMN_WIDTH = 240,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 120,
        MINIMUM_COLUMN_WIDTH = 23
    };

    void setSearchWidget(QComboBox*,QComboBox*,QLineEdit*);
private:
    WalletModel *model;
    TransactionFilterProxy *transactionProxyModel;
    QTableView *transactionView;
    QComboBox *dateWidget;
    QComboBox *typeWidget;
    QComboBox *watchOnlyWidget;
    // QLineEdit *addressWidget;
    // QLineEdit *amountWidget;

    QMenu *contextMenu;
    QSignalMapper *mapperThirdPartyTxUrls;

    QFrame *dateRangeWidget;
    MDateEdit *dateFrom; //MDateEdit
    MDateEdit *dateTo;
    QAction *abandonAction;

    QWidget *createDateRangeWidget();

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

    bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void showDetails();
    void copyAddress();
    void editLabel();
    void copyLabel();
    void copyAmount();
    void copyTxID();
    void copyTxHex();
    void copyTxPlainText();
    void openThirdPartyTxUrl(QString url);
    void updateWatchOnlyColumn(bool fHaveWatchOnly);
    void abandonTx();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

    /** Send computed sum back to wallet-view */
    void trxAmount(QString amount);

public Q_SLOTS:
    void chooseDate(int idx);
    void chooseType(int idx);
    void chooseWatchonly(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void exportClicked();
    void focusTransaction(const QModelIndex&);
    void computeSum();
};


class DefineCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    DefineCalendar(QWidget *parent);
    ~DefineCalendar();

Q_SIGNALS:
    void setFinished(const QDateTime &dateTime);

public Q_SLOTS:
    void UpdateYear();
    void UpdatePage();
    void SetToday();
    void ClearTime();

protected Q_SLOTS:
    void BtnSlots();
    void ComboBoxSlots(int index);
    void CurPageChange(int year, int month);

protected:
    void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const;

private:
    void InitWidgets();
    QWidget *widget_top;
    QPushButton *pushBtn_YL;
    QComboBox *comboBox_Year;
    QPushButton *pushBtn_YR;
    QPushButton *pushBtn_ML;
    QComboBox *comboBox_Month;
    QPushButton *pushBtn_MR;
};


class MDateEdit : public QDateEdit
{
    Q_OBJECT

public:
    MDateEdit(QWidget *parent);
    ~MDateEdit();
    void setMyStytle();

protected Q_SLOTS:
    void getDateTime(const QDateTime &dateTime);

private:
    DefineCalendar *m_DefCalendar;

};


class MItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    MItemDelegate(QObject *parent=0);
    ~MItemDelegate();

private:

    void paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};



#endif // MASSGRID_QT_TRANSACTIONVIEW_H
