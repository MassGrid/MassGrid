#ifndef MASTERNODELIST_H
#define MASTERNODELIST_H

#include "primitives/transaction.h"
#include "platformstyle.h"
#include "sync.h"
#include "util.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>
#include <QTableWidget>

#define MY_MASTERNODELIST_UPDATE_SECONDS                 60
#define MASTERNODELIST_UPDATE_SECONDS                    15
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS            3

namespace Ui {
    class MasternodeList;
}

class ClientModel;
class WalletModel;
class QTableWidgetItem;
class QSwitchButton;
class Task;
class Service;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Masternode Manager page widget */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    explicit MasternodeList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MasternodeList();

    enum DockerUpdateMode{
        AfterCreate = 0,
        WhenNormal
    };

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");

    static void getEdgeRet(bool flag);
    void showEdgeRet(bool flag);

private:
    QMenu *contextMenu;
    int64_t nTimeFilterUpdated;
    bool fFilterUpdated;

public Q_SLOTS:
    void updateMyMasternodeInfo(QString strAlias, QString strAddr, const COutPoint& outpoint);
    void updateMyNodeList(bool fForce = false);
    void updateNodeList();
    void refreshServerList();

Q_SIGNALS:

private:
    QTimer *timer;
    QTimer *m_serviceTimer;
    Ui::MasternodeList *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

    // Protects tableWidgetMasternodes
    CCriticalSection cs_mnlist;

    // Protects tableWidgetMyMasternodes
    CCriticalSection cs_mymnlist;

    QString strCurrentFilter;

    std::string m_curAddr_Port;

    DockerUpdateMode m_updateMode;
    int timeOutSec;
    
    QSwitchButton *switchButton;

    int64_t m_nTimeDockerListUpdated;
    int64_t m_nTimeListUpdated;
    int64_t m_nTimeMyListUpdated;

private:
    int loadServerList();
    bool deleteService(const std::string& serviceid);
    void clearDockerDetail();
    void setCurUpdateMode(DockerUpdateMode mode);
    DockerUpdateMode getCurUpdateMode();
    void startTimer(bool start);
    void askDNData();
    void updateEdgeStatus(int count =0);
    bool getVirtualIP(const QString& n2n_localip,QString& virtualIP);
    void initOrderTablewidget();
    void resetTableWidgetTitle();
    void loadOrderTableWidget();

protected:
    void resizeEvent(QResizeEvent *event);


private Q_SLOTS:
    void showContextMenu(const QPoint &);
    void on_filterLineEdit_textChanged(const QString &strFilterIn);
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyMasternodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
    void showDockerDetail(QModelIndex);
    void loadServerDetail(QModelIndex);
    void slot_updateServiceBtn();
    void slot_deleteServiceBtn();
    void updateServiceList();
    void openServiceDetail(QModelIndex);

    void loadDockerDetail(const std::string& key);
    // void updateServiceDetail(Service& service);
    // void updateTaskDetail(std::map<std::string,Task> &mapDockerTasklists,int& taskStatus);
    void slot_createServiceBtn();
    void slot_changeN2Nstatus(bool);
    void slot_curTabPageChanged(int);
    void slot_ordertablecheck();
    void updateDockerList(bool fForce = false);
};
#endif // MASTERNODELIST_H
