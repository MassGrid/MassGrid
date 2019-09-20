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
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS            2

namespace Ui {
    class MasternodeList;
}

class ClientModel;
class WalletModel;
class QTableWidgetItem;
class QSwitchButton;
class Task;
class DockerUpdateService;
class AskServicesWorker;
class DockerOrderView;

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

    enum MoasternodeColumnIndex {
        Address = 0,
        Protocol = 1,
        Status = 2,
        Active = 3,
        LastSeen = 4,
        Pubkey = 5,
        ActiveCount = 6,
        JoinToken = 7
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
    void updateDockerOrder();

Q_SIGNALS:

private:
    QTimer *timer;
    Ui::MasternodeList *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    DockerOrderView *dockerorderView;

    // Protects tableWidgetMasternodes
    CCriticalSection cs_mnlist;

    // Protects tableWidgetMyMasternodes
    CCriticalSection cs_mymnlist;

    QString strCurrentFilter;

    std::string m_curAddr_Port;
    std::string m_curserviceID;

    DockerUpdateMode m_updateMode;
    int timeOutSec;
    
    QSwitchButton *switchButton;

    QTimer* m_scanTimer;

    int64_t m_nTimeDockerListUpdated;
    int64_t m_nTimeListUpdated;
    int64_t m_nTimeMyListUpdated;

    DockerUpdateService *m_updateService{};
    AskServicesWorker *m_askServiceDataWorker;
    
private:
    int loadServerList();
    void clearDockerDetail();
    void setCurUpdateMode(DockerUpdateMode mode);
    DockerUpdateMode getCurUpdateMode();
    void startTimer(bool start);
    void updateEdgeStatus(int count =0);
    bool getVirtualIP(const QString& n2n_localip,const QString& n2n_netmask,QString& virtualIP);
    void initOrderTablewidget();
    void resetTableWidgetTitle();
    void jumpToCheckOrder(int index);

    void gotoDockerSerivcePage(const std::string& str);
    void gotoCreateServicePage(const std::string& str,const std::string& txid);
    void gotoOrderDetailPage(int index);
    void initDockerOrderView(const PlatformStyle *platformStyle);
    void startScanTimer(int msec);
    void fullRerentServiceData(const COutPoint& createOutPoint, const COutPoint& outPoint);
    void doLoadServiceTask();
    void startAskServiceDataWork(const char* slotMethod,bool needAsk);
    void updateService();

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
    // void updateServiceList();
    void openServiceDetail(QModelIndex);
    // void askDNData();
    void askServiceData();
    void askRerentServiceData();

    int loadDockerDetail(const std::string& key);
    void slot_createServiceBtn();
    void slot_changeN2Nstatus(bool);
    void slot_curTabPageChanged(int);
    void updateDockerList(bool fForce = false);

    void slot_btn_refund();
    void loadOrderData();
    void dockerOrderViewdoubleClicked(QModelIndex index);
    void dockerOrderViewitemClicked(const QModelIndex index);
    void deleteService(COutPoint& outpoint,std::string ip_port);
    void jumpToCheckService(std::string ip);
    void jumpToCreateService(std::string ip,std::string txid);
    void timeoutToScanStatus();
    void disenableDeleteServiceBtn();
    void onPBtn_rerentClicked();
    void slot_doRerentService();
    // void askTransData(std::string txid);
    void updateServiceListFinished(bool isTaskFinished);
    void updateRerentServiceFinished(bool isTaskFinished);
};

#endif // MASTERNODELIST_H
