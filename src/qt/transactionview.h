// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
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

class TransactionFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
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
    explicit TransactionView(QWidget *parent = 0);

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
        STATUS_COLUMN_WIDTH = 23,
        WATCHONLY_COLUMN_WIDTH = 23,
        DATE_COLUMN_WIDTH = 120,
        TYPE_COLUMN_WIDTH = 120,
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

    QWidget *createDateRangeWidget();

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void showDetails();
    void copyAddress();
    void editLabel();
    void copyLabel();
    void copyAmount();
    void copyTxID();
    void openThirdPartyTxUrl(QString url);
    void updateWatchOnlyColumn(bool fHaveWatchOnly);

signals:
    void doubleClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

public slots:
    void chooseDate(int idx);
    void chooseType(int idx);
    void chooseWatchonly(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void exportClicked();
    void focusTransaction(const QModelIndex&);
};




class DefineCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    DefineCalendar(QWidget *parent);
    ~DefineCalendar();

signals:
    void setFinished(const QDateTime &dateTime);

public slots:
    void UpdateYear();
    void UpdatePage();
    void SetToday();
    void ClearTime();

protected slots:
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

protected slots:
    void getDateTime(const QDateTime &dateTime);

private:
    DefineCalendar *m_DefCalendar;

};



#endif // MASSGRID_QT_TRANSACTIONVIEW_H
