// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_DOCKERORDERVIEW_H
#define MASSGRID_QT_DOCKERORDERVIEW_H

#include "guiutil.h"

#include <QWidget>
#include <QKeyEvent>

#include <QDateTimeEdit>
#include <QDateTime>
#include <QDateEdit>
#include <QCalendarWidget>

#include <QObject>
#include <qstyleditemdelegate.h>
#include <QMap>

#include <QMutexLocker>
#include <QWaitCondition>
#include <QThread>

class PlatformStyle;
class DockerOrderFilterProxy;
class WalletModel;

// class QWaitCondition;
// class QThread;
// class QMutex;

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
class SyncTransactionHistoryThread;
class QLabel;
class QComboBox;
class QPushButton;
class QTimeEdit;
class QDateTime;
class CWalletTx;
QT_END_NAMESPACE

/** Widget showing the transaction list for a wallet, including a filter row.
    Using the filter row, the user can view or export a subset of the transactions.
  */
class DockerOrderView : public QWidget
{
    Q_OBJECT

public:
    explicit DockerOrderView(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~DockerOrderView();
    void setModel(WalletModel *model);
    // void getCurrentItemTxidAndmnIp(std::string &txid,std::string &masternodeip,std::string &orderStatus);
    void getCurrentItemTxidAndmnIp(std::string &txid);
    
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
        TYPE_COLUMN_WIDTH = 200,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 120,
        MINIMUM_COLUMN_WIDTH = 23
    };

    void setSearchWidget(QComboBox*,QComboBox*,QLineEdit*);
    void refreshModel();

private:
    WalletModel *model;
    DockerOrderFilterProxy *dockerorderProxyModel;
    QTableView *dockerorderView;
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
    bool getOrderBtnText(CWalletTx& wtx,QString& btnText)const;

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;
    SyncTransactionHistoryThread *m_syncTransactionThread;

    virtual void resizeEvent(QResizeEvent* event);
    bool eventFilter(QObject *obj, QEvent *event);
    void updateTransData(QString txid,bool isAskAll);
    void stopAndDeleteSyncThread();
    void addTransTask(const std::string& txidStr);
    bool updateOrderStatus(const std::string&)const;
 
private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    // void showDetails();
    void showOrderDetail(std::string txid,CWalletTx& wtx);
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
    void updateTransactionHistoryData(const QString&,bool);

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);
    void itemClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

    /** Send computed sum back to wallet-view */
    void trxAmount(QString amount);
    void deleteService(std::string txid,std::string ip_port);
    void openServicePage(std::string masternodeip);
    void gotoCreateServicePage(std::string,std::string);

public Q_SLOTS:
    void chooseDate(int idx);
    void chooseType(int idx);
    void chooseWatchonly(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void exportClicked();
    void focusTransaction(const QModelIndex&);
    void computeSum();
    void deleteService();
    void slot_btnClicked();
    void updateOrder(const std::string&);
    void addOperationBtn(int)const;
    void deleteTransaction(int)const;
    void updateAllOperationBtn();
    void txidPrefix(const QString &prefix);

    // void slot_BtnDestroyed(QObject*);

};

class SyncTransactionHistoryThread : public QThread
{
    Q_OBJECT
public:
    explicit SyncTransactionHistoryThread(QObject *parent = 0);
    ~SyncTransactionHistoryThread();

    void addTask(const QString& txid,bool isAskAll);
    void setNeedWork(bool type);

private:
    bool m_isNeedWork = true;
    // QList<QString> m_taskList;
    QList<QPair<QString,bool>> m_taskList;
    QList<QString> m_finishedTasks;

    QWaitCondition m_wait;
    QMutex m_mutex;

private:
    bool isNeedWork();
    bool popTask(QString& txid,bool& isAskAll);
    bool doTask(const QString& txid,bool isAskAll);

protected:
    void run();

Q_SIGNALS:
    void syncTaskEnd(const QString& txid,bool);
    void syncThreadFinished();
    void syncTimeOut();
};

#endif // MASSGRID_QT_DOCKERORDERVIEW_H
