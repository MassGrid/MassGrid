#include "masternodelist.h"
#include "ui_masternodelist.h"

#include "activemasternode.h"
#include "clientmodel.h"
#include "init.h"
#include "guiutil.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QTimer>
#include <QTableWidgetItem>
#include <QListWidgetItem>

#include <boost/random.hpp>
#include "cmessagebox.h"
#include "dockercluster.h"
#include "dockerman.h"
#include "util.h"
#include "adddockerservicedlg.h"
#include "massgridgui.h"
#include "dockeredge.h"
#include "qswitchbutton.h"
#include "dockerserverman.h"
#include "masternode-sync.h"
#include <QLabel>
#include <QStringList>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QList>
#include "servicedetail.h"
#include "orderdetail.h"
#include "dockerorderview.h"
#include "transactionview.h"
#include "optionsmodel.h"

#define DOCKER_AFTERCREATE_UPDATE_SECONDS 5
#define DOCKER_WHENNORMAL_UPDATE_SECONDS 600

extern MasternodeList* g_masternodeListPage;
extern CDockerServerman dockerServerman;

int GetOffsetFromUtc()
{
#if QT_VERSION < 0x050200
    const QDateTime dateTime1 = QDateTime::currentDateTime();
    const QDateTime dateTime2 = QDateTime(dateTime1.date(), dateTime1.time(), Qt::UTC);
    return dateTime1.secsTo(dateTime2);
#else
    return QDateTime::currentDateTime().offsetFromUtc();
#endif
}

MasternodeList::MasternodeList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0),
    m_scanTimer(NULL)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

    ui->tableWidgetMyMasternodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetMasternodes->hideColumn(MasternodeList::Pubkey);
    ui->tableWidgetMasternodes->hideColumn(MasternodeList::JoinToken);

    ui->tableWidgetMasternodes->verticalHeader()->setVisible(false); 

    ui->tableWidgetMasternodes->setSortingEnabled(true);
    ui->tableWidgetMasternodes->sortByColumn(MasternodeList::ActiveCount, Qt::DescendingOrder);

    ui->serviceTableWidget->setColumnWidth(0, 150);
    ui->serviceTableWidget->setColumnWidth(1, 100);

    ui->serviceTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->serviceTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->serviceTableWidget->verticalHeader()->setVisible(false); 

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *startAliasAction = new QAction(tr("Start alias"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->tableWidgetMasternodes, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDockerDetail(QModelIndex)));
    connect(ui->serviceTableWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(loadServerDetail(QModelIndex)));
    connect(ui->serviceTableWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openServiceDetail(QModelIndex)));
    
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    connect(ui->updateServiceBtn, SIGNAL(clicked()), this, SLOT(slot_updateServiceBtn()));
    connect(ui->deleteServiceBtn, SIGNAL(clicked()), this, SLOT(slot_deleteServiceBtn()));
    
    connect(ui->createServiceBtn, SIGNAL(clicked()), this, SLOT(slot_createServiceBtn()));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)),this,SLOT(slot_curTabPageChanged(int)));
    connect(ui->pushButton_refund,SIGNAL(clicked()),this,SLOT(slot_btn_refund()));
    connect(ui->lineEdit_searchOrder,SIGNAL(),this,SLOT());

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
    updateNodeList();
    ui->tabWidget->tabBar()->setTabEnabled(2,false);

    switchButton = new QSwitchButton(ui->frame_2);
    switchButton->SetSelected(false);
    switchButton->SetSize(120,32);
    connect(switchButton,SIGNAL(clicked(bool)),this,SLOT(slot_changeN2Nstatus(bool)));
    clearDockerDetail();
    resetTableWidgetTitle();

    QTimer::singleShot(3000,this,SLOT(update()));
    ui->tabWidget->setCurrentIndex(0);

    initDockerOrderView(platformStyle);

    // ui->pBtn_searchOrderBtn->hide();
    // ui->lineEdit_searchOrder->hide();
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::resizeEvent(QResizeEvent *event)
{
    resetTableWidgetTitle();
    QWidget::resizeEvent(event);
}

void MasternodeList::resetTableWidgetTitle()
{
    int itemwidth = ui->tableWidgetMasternodes->width()/6;
    int columnCount = ui->tableWidgetMasternodes->columnCount();

    for(int i=0;i<columnCount;i++){
        ui->tableWidgetMasternodes->setColumnWidth(i, itemwidth);
    }

    ui->tableWidgetMasternodes->hideColumn(MasternodeList::Pubkey);
    ui->tableWidgetMasternodes->hideColumn(MasternodeList::JoinToken);

    itemwidth = ui->serviceTableWidget->width()/5;
    columnCount = ui->serviceTableWidget->columnCount();

    for(int i=0;i<columnCount;i++){
        ui->serviceTableWidget->setColumnWidth(i, itemwidth);
    }
}

void MasternodeList::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model) {
        // try to update list when masternode count changes
        connect(clientModel, SIGNAL(strMasternodesChanged(QString)), this, SLOT(updateNodeList()));
    }
}

void MasternodeList::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    dockerorderView->setModel(model);
}

void MasternodeList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetMyMasternodes->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void MasternodeList::showDockerDetail(QModelIndex index)
{
    if (!masternodeSync.IsSynced()){
        CMessageBox::information(this, tr("Docker option"),tr("Can't open docker detail page without synced!"));
        return;
    }

    int row = index.row();
    const std::string& address_port = ui->tableWidgetMasternodes->item(row,0)->text().toStdString().c_str();
    
    gotoDockerSerivcePage(address_port);
}

void MasternodeList::gotoDockerSerivcePage(const std::string& ip_port)
{
    // check n2n status
    if(switchButton->IsSelected() && m_curAddr_Port.size() > 0 && m_curAddr_Port != ip_port){

        QString msg = tr("Edge is running,the last masternode ip is:%1,we will close it if you want to switch to this node,are you sure that?")
                        .arg(QString::fromStdString(m_curAddr_Port));
        CMessageBox::StandardButton retval = CMessageBox::question(this, tr("N2N Status"),msg,
            CMessageBox::Ok_Cancel,
            CMessageBox::Cancel);

        if(retval != CMessageBox::Ok)
            return;
        else
            updateEdgeStatus(0);
    }
    m_curAddr_Port = ip_port;
    ui->tabWidget->setCurrentIndex(2);
    setCurUpdateMode(DockerUpdateMode::WhenNormal);
    updateServiceList();
}

void MasternodeList::StartAlias(std::string strAlias)
{
    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;

    BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        if(mne.getAlias() == strAlias) {
            std::string strError;
            CMasternodeBroadcast mnb;

            bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            if(fSuccess) {
                strStatusHtml += "<br>Successfully started masternode.";
                mnodeman.UpdateMasternodeList(mnb, *g_connman);
                mnb.Relay(*g_connman);
                mnodeman.NotifyMasternodeUpdates(*g_connman);
            } else {
                strStatusHtml += "<br>Failed to start masternode.<br>Error: " + strError;
            }
            break;
        }
    }
    strStatusHtml += "</center>";

    CMessageBox::information(this, tr(""),
                        QString::fromStdString(strStatusHtml));

    updateMyNodeList(true);
}

void MasternodeList::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;

    BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        std::string strError;
        CMasternodeBroadcast mnb;

        int32_t nOutputIndex = 0;
        if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), nOutputIndex);

        if(strCommand == "start-missing" && mnodeman.Has(outpoint)) continue;

        bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

        if(fSuccess) {
            nCountSuccessful++;
            mnodeman.UpdateMasternodeList(mnb, *g_connman);
            mnb.Relay(*g_connman);
            mnodeman.NotifyMasternodeUpdates(*g_connman);
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d masternodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    CMessageBox::information(this, tr(""),
                    QString::fromStdString(returnObj));


    updateMyNodeList(true);
}

void MasternodeList::updateMyMasternodeInfo(QString strAlias, QString strAddr, const COutPoint& outpoint)
{
    bool fOldRowFound = false;
    int nNewRow = 0;

    for(int i = 0; i < ui->tableWidgetMyMasternodes->rowCount(); i++) {
        if(ui->tableWidgetMyMasternodes->item(i, 0)->text() == strAlias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if(nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMyMasternodes->rowCount();
        ui->tableWidgetMyMasternodes->insertRow(nNewRow);
    }

    masternode_info_t infoMn;
    bool fFound = mnodeman.GetMasternodeInfo(outpoint, infoMn);

    QTableWidgetItem *aliasItem = new QTableWidgetItem(strAlias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(fFound ? QString::fromStdString(infoMn.addr.ToString()) : strAddr);
    QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(fFound ? infoMn.nProtocolVersion : -1));
    QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(fFound ? CMasternode::StateToString(infoMn.nActiveState) : "MISSING"));
    QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(fFound ? (infoMn.nTimeLastPing - infoMn.sigTime) : 0)));
    QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M",
                                                                                                   fFound ? infoMn.nTimeLastPing + GetOffsetFromUtc() : 0)));
    QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(fFound ? CMassGridAddress(infoMn.pubKeyCollateralAddress.GetID()).ToString() : ""));

    ui->tableWidgetMyMasternodes->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 2, protocolItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 3, statusItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 4, activeSecondsItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 5, lastSeenItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 6, pubkeyItem);
}

void MasternodeList::updateMyNodeList(bool fForce)
{
    TRY_LOCK(cs_mymnlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }
    // static int64_t nTimeMyListUpdated = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click

    int64_t nSecondsTillUpdate = m_nTimeMyListUpdated + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if(nSecondsTillUpdate > 0 && !fForce) return;
    m_nTimeMyListUpdated = GetTime();

    ui->tableWidgetMasternodes->setSortingEnabled(false);
    BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
        int32_t nOutputIndex = 0;
        if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        updateMyMasternodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), COutPoint(uint256S(mne.getTxHash()), nOutputIndex));
    }
    ui->tableWidgetMasternodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void MasternodeList::updateNodeList()
{
    TRY_LOCK(cs_mnlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    // static int64_t m_nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS seconds
    // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                            ? nTimeFilterUpdated - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS
                            : m_nTimeListUpdated - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;

    m_nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetMasternodes->setSortingEnabled(false);
    ui->tableWidgetMasternodes->clearContents();
    ui->tableWidgetMasternodes->setRowCount(0);
    std::map<COutPoint, CMasternode> mapMasternodes = mnodeman.GetFullMasternodeMap();
    int offsetFromUtc = GetOffsetFromUtc();

    for(auto& mnpair : mapMasternodes)
    {
        CMasternode mn = mnpair.second;
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.nProtocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.GetStatus()));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)));
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", mn.lastPing.sigTime + offsetFromUtc)));
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(CMassGridAddress(mn.pubKeyCollateralAddress.GetID()).ToString()));
        QTableWidgetItem *nodeCount = new QTableWidgetItem(QString::number(mn.lastPing.mdocker.activeNodeCount) + "/" +QString::number(mn.lastPing.mdocker.nodeCount)); 
        QTableWidgetItem *joinToken = new QTableWidgetItem(QString::fromStdString(mn.lastPing.mdocker.joinToken));
        
        if (strCurrentFilter != "")
        {
            strToFilter =   addressItem->text() + " " +
                            protocolItem->text() + " " +
                            statusItem->text() + " " +
                            activeSecondsItem->text() + " " +
                            lastSeenItem->text() + " " +
                            pubkeyItem->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetMasternodes->insertRow(0);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::Address, addressItem);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::Protocol, protocolItem);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::Status, statusItem);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::Active, activeSecondsItem);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::LastSeen, lastSeenItem);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::Pubkey, pubkeyItem);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::ActiveCount, nodeCount);
        ui->tableWidgetMasternodes->setItem(0, MasternodeList::JoinToken, joinToken); 

        for(int i=0;i<8;i++)
            ui->tableWidgetMasternodes->item(0,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);            
    }

    ui->countLabel->setText(QString::number(ui->tableWidgetMasternodes->rowCount()));
    ui->tableWidgetMasternodes->setSortingEnabled(true);
}

void MasternodeList::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void MasternodeList::on_startButton_clicked()
{
    std::string strAlias;
    {
        LOCK(cs_mymnlist);
        // Find selected node alias
        QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if(selected.count() == 0) return;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        strAlias = ui->tableWidgetMyMasternodes->item(nSelectedRow, 0)->text().toStdString();
    }

    // Display message box
    CMessageBox::StandardButton retval = CMessageBox::question(this, tr("Confirm masternode start"),
        tr("Are you sure you want to start masternode %1?").arg(QString::fromStdString(strAlias)),
        CMessageBox::Ok_Cancel,
        CMessageBox::Cancel);

    if(retval != CMessageBox::Ok) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAlias(strAlias);
        return;
    }

    StartAlias(strAlias);
}

void MasternodeList::on_startAllButton_clicked()
{
    // Display message box
    CMessageBox::StandardButton retval = CMessageBox::question(this, tr("Confirm all masternodes start"),
        tr("Are you sure you want to start ALL masternodes?"),
        CMessageBox::Ok_Cancel,
        CMessageBox::Cancel);

    if(retval != CMessageBox::Ok) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll();
        return;
    }

    StartAll();
}

void MasternodeList::on_startMissingButton_clicked()
{
    if(!masternodeSync.IsMasternodeListSynced()) {
        CMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until masternode list is synced"));
        return;
    }

    // Display message box
    CMessageBox::StandardButton retval = CMessageBox::question(this,
        tr("Confirm missing masternodes start"),
        tr("Are you sure you want to start MISSING masternodes?"),
        CMessageBox::Ok_Cancel,
        CMessageBox::Cancel);

    if(retval != CMessageBox::Ok) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll("start-missing");
        return;
    }

    StartAll("start-missing");
}

void MasternodeList::on_tableWidgetMyMasternodes_itemSelectionChanged()
{
    if(ui->tableWidgetMyMasternodes->selectedItems().count() > 0) {
        ui->startButton->setEnabled(true);
    }
}

void MasternodeList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void MasternodeList::slot_curTabPageChanged(int curPage)
{
    startTimer(false);
    resetTableWidgetTitle();
    if(curPage <=1){
        disconnect(timer, SIGNAL(timeout()), this, SLOT(updateDockerList()));
        connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
        connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
        m_nTimeListUpdated = GetTime();
        m_nTimeMyListUpdated = GetTime();
    }
    else if(curPage == 2){
        connect(timer, SIGNAL(timeout()), this, SLOT(updateDockerList()));
        disconnect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
        disconnect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
        setCurUpdateMode(DockerUpdateMode::WhenNormal);
    }
    else if(curPage == 3){
        if (!masternodeSync.IsSynced()){
            CMessageBox::information(this, tr("Docker option"),tr("Can't open docker order page without synced!"));
            ui->tabWidget->setCurrentIndex(0);
            return;
        }
        dockerorderView->updateAllOperationBtn();
    }
    startTimer(true);
}

void MasternodeList::startTimer(bool start)
{
    if(start){
        timer->start(1000);
    }
    else{
        timer->stop();
    }
}

//timeout
void MasternodeList::updateDockerList(bool fForce)
{
    int64_t nSecondsTillUpdate = 0;
    
    if(m_updateMode == DockerUpdateMode::AfterCreate){
        nSecondsTillUpdate = m_nTimeDockerListUpdated + DOCKER_AFTERCREATE_UPDATE_SECONDS - GetTime();
        ui->serviceSec->setText(QString::number(nSecondsTillUpdate));
    }
    else{
        nSecondsTillUpdate = m_nTimeDockerListUpdated + DOCKER_WHENNORMAL_UPDATE_SECONDS - GetTime();
        QTime time(nSecondsTillUpdate/3600,nSecondsTillUpdate/60,nSecondsTillUpdate%60);

        ui->serviceSec->setText(time.toString("mm:ss"));
    }

    if(nSecondsTillUpdate >= 0 && !fForce) return;
    m_nTimeDockerListUpdated = GetTime();

    // setCurUpdateMode(DockerUpdateMode::WhenNormal);

    if(getCurUpdateMode() == DockerUpdateMode::AfterCreate)
            askDNData();
    else if(DockerUpdateMode::WhenNormal)
        updateServiceList();
}

int MasternodeList::loadServerList()
{
    ui->serviceTableWidget->setRowCount(0);

    std::map<std::string,Service> serverlist = dockercluster.dndata.mapDockerServiceLists;
    std::map<std::string,Service>::iterator iter = serverlist.begin();

    int count = 0;
    for(;iter != serverlist.end();iter++){
        QString id = QString::fromStdString(iter->first);
        Service service = serverlist[iter->first];
        QString name = QString::fromStdString(service.spec.name);
        map<std::string,Task> mapDockerTasklists = service.mapDockerTaskLists;

        uint64_t createdAt = service.createdAt;
        // std::string name = service.spec.name;
        // std::string address = service.spec.labels["com.massgrid.pubkey"];
        std::string image = service.spec.taskTemplate.containerSpec.image;

        int taskStatus = -1;
        QString taskStatusStr = tr("Waiting");
        if(mapDockerTasklists.size() > 0){
            Task task = mapDockerTasklists.begin()->second;
            taskStatus = task.status.state;
            taskStatusStr = taskStatus == Config::TASKSTATE_RUNNING ? tr("Create completed") : tr("Creating...");
        }

        QLabel *label = new QLabel(ui->serviceTableWidget);

        label->setText(taskStatusStr);
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        label->setStyleSheet(taskStatus == 8 ? "color:green;" : "color:red;");
 
        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        QTableWidgetItem *imageItem = new QTableWidgetItem(QString::fromStdString(image));
        QTableWidgetItem *idItem = new QTableWidgetItem(id);
        QTableWidgetItem *timeoutItem;

        CWalletTx& wtx = pwalletMain->mapWallet[service.txid];
        if(wtx.Getprice().size()){
            CAmount itemPrice = (CAmount)(QString::fromStdString(wtx.Getprice()).toDouble());
            CAmount payment = GUIUtil::getTxidAmount(service.txid.ToString())*(-1);

            QString itemPriceStr = MassGridUnits::formatWithUnit(MassGridUnits::MGD, itemPrice);
            QString paymentStr = MassGridUnits::formatWithUnit(MassGridUnits::MGD, payment);

            if(itemPriceStr.split(" ").size()==2 && paymentStr.split(" ").size()==2){
                double itemPrice_num = itemPriceStr.split(" ").at(0).toDouble();
                double payment_num = paymentStr.split(" ").at(0).toDouble();
                int msec = (payment_num/itemPrice_num)*3600 + fmod(payment_num,itemPrice_num)*60 ;
                timeoutItem = new QTableWidgetItem(QDateTime::fromTime_t(createdAt).addSecs(msec).toString("yyyy-MM-dd hh:mm:ss"));
            }
            else
            {
                timeoutItem = new QTableWidgetItem(QDateTime::fromTime_t(createdAt).addSecs(3600).toString("yyyy-MM-dd hh:mm:ss"));
            }
        }

        ui->serviceTableWidget->insertRow(count);
        ui->serviceTableWidget->setItem(count, 0, idItem);
        ui->serviceTableWidget->setItem(count, 1, nameItem);
        ui->serviceTableWidget->setItem(count, 2, imageItem);
        ui->serviceTableWidget->setItem(count, 3, timeoutItem);
        ui->serviceTableWidget->setCellWidget(count,4,label);

        for(int i=0;i<4;i++)
            ui->serviceTableWidget->item(count,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);            

        count++;
    }
    dockerServerman.setDNDataStatus(CDockerServerman::Free);

    return count;
}

void MasternodeList::loadServerDetail(QModelIndex index)
{
    QString key = ui->serviceTableWidget->item(index.row(),0)->text();
    int taskStatus = loadDockerDetail(key.toStdString());

    if(taskStatus == Config::TASKSTATE_RUNNING){
        ui->deleteServiceBtn->setEnabled(true);
        switchButton->setEnabled(true);
    }
}

void MasternodeList::disenableDeleteServiceBtn()
{
    ui->deleteServiceBtn->setEnabled(false);
    switchButton->setEnabled(false);
}

int MasternodeList::loadDockerDetail(const std::string & key)
{
    Service service = dockercluster.dndata.mapDockerServiceLists[key.c_str()];
    map<std::string,Task> mapDockerTasklists = service.mapDockerTaskLists;

    int taskStatus = -1;
    map<std::string,Task>::iterator iter = mapDockerTasklists.begin();
    for(;iter != mapDockerTasklists.end();iter++){
        Task task = iter->second;
        taskStatus = task.status.state;
    }

    DockerUpdateMode mode;
    //  = taskStatus == 8 ? DockerUpdateMode::WhenNormal : DockerUpdateMode::AfterCreate;
    if(taskStatus == Config::TASKSTATE_RUNNING)
        mode = DockerUpdateMode::WhenNormal;
    else
        mode = DockerUpdateMode::AfterCreate;
    
    setCurUpdateMode(mode);

    return taskStatus;
}

void MasternodeList::openServiceDetail(QModelIndex index)
{
    QString key = ui->serviceTableWidget->item(index.row(),0)->text();

    Service service = dockercluster.dndata.mapDockerServiceLists[key.toStdString().c_str()];

    ServiceDetail dlg;

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);

    dlg.setModel(walletModel);

    dlg.setService(service);
    dlg.exec();
}

void MasternodeList::slot_updateServiceBtn()
{
    setCurUpdateMode(DockerUpdateMode::WhenNormal);
    updateDockerList(true);
}

void MasternodeList::slot_deleteServiceBtn()
{
    int curindex = ui->serviceTableWidget->currentRow();
    
    if(curindex <0 )
        return ;

    std::string serviceid = ui->serviceTableWidget->item(curindex,0)->text().toStdString();
    std::string txid = dockercluster.dndata.mapDockerServiceLists[serviceid].txid.ToString();

    deleteService(txid,m_curAddr_Port);
    QTimer::singleShot(2000,this,SLOT(slot_updateServiceBtn()));
}

void MasternodeList::deleteService(std::string txid,std::string ip_port)
{
    CMessageBox::StandardButton retval = CMessageBox::question(this, tr("Delete Service"),
        tr("Are you sure you want to delete this service?"),
        CMessageBox::Ok_Cancel,
        CMessageBox::Cancel);
    
    if(retval != CMessageBox::Ok) return;

    if(!dockercluster.SetConnectDockerAddress(ip_port) || !dockercluster.ProcessDockernodeConnections()){
        CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!"));
        LogPrintf("MasternodeList deleteService failed\n");
    }

    DockerDeleteService delService{};

    delService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
    delService.txid = uint256S(txid); //dockercluster.dndata.mapDockerServiceLists[txid].txid;

    if(!dockercluster.DeleteAndSendServiceSpec(delService)){
        CMessageBox::information(this, tr("Docker option"),tr("Delete docker service failed!"));
    }
    LogPrintf("MasternodeList deleteService sucess\n");
}

void MasternodeList::updateServiceList()
{
    if(dockercluster.SetConnectDockerAddress(m_curAddr_Port) && dockercluster.ProcessDockernodeConnections()){
        clearDockerDetail();
        askDNData();
    }
    else{
        CMessageBox::information(this, tr("Docker option"),tr("Connect docker network failed!"));
    }
}

void MasternodeList::askDNData()
{
    dockercluster.AskForDNData();
    refreshServerList();
}

void MasternodeList::clearDockerDetail()
{
    ui->deleteServiceBtn->setEnabled(false);
    switchButton->setEnabled(false);
    ui->serviceTableWidget->setRowCount(0);
}

void MasternodeList::slot_createServiceBtn()
{
    std::string ip_port = m_curAddr_Port;
    gotoCreateServicePage(ip_port,"");
}

void MasternodeList::gotoCreateServicePage(const std::string& ip_port,const std::string& txid)
{
    gotoDockerSerivcePage(ip_port);

    AddDockerServiceDlg dlg;

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
    dlg.setaddr_port(ip_port);
    dlg.settxid(txid);
    dlg.setWalletModel(walletModel);

    if(dlg.exec() == QDialog::Accepted){
        setCurUpdateMode(DockerUpdateMode::AfterCreate);
        refreshServerList();  
    }
}

void MasternodeList::refreshServerList()
{
    static int refreshCount = 0 ;
    int currentIndex = ui->tabWidget->currentIndex();

    if(currentIndex != 2)
        return ;

    if(dockerServerman.getDNDataStatus() == CDockerServerman::Ask){

        if(DockerUpdateMode::WhenNormal)
            QTimer::singleShot(2000,this,SLOT(refreshServerList()));
        LogPrintf("MasternodeList get DNData Status:CDockerServerman::Ask\n");
        return ;
    }
    else if(dockerServerman.getDNDataStatus() == CDockerServerman::Received ||
            dockerServerman.getDNDataStatus() == CDockerServerman::Free){
        int count = loadServerList();
        if(!count){
            clearDockerDetail();
            setCurUpdateMode(DockerUpdateMode::WhenNormal);
        }
        else{
            int rowCount = ui->serviceTableWidget->rowCount();
            for(int i=0;i<rowCount;i++){
                QString key = ui->serviceTableWidget->item(i,0)->text();
                int taskStatus = loadDockerDetail(key.toStdString());
                if(taskStatus != Config::TASKSTATE_RUNNING){
                    setCurUpdateMode(DockerUpdateMode::AfterCreate);
                    QTimer::singleShot(2000,this,SLOT(askDNData()));
                    break;
                }
            }
        }
        updateEdgeStatus(count);
        LogPrintf("MasternodeList get DNData Status:CDockerServerman::Received\n");
        return ;
    }
}

void MasternodeList::setCurUpdateMode(DockerUpdateMode mode)
{
    m_updateMode = mode;
    if(mode == DockerUpdateMode::AfterCreate){
        ui->updateServiceBtn->setEnabled(false);
    }
    else{
        ui->updateServiceBtn->setEnabled(true);
    }
    m_nTimeDockerListUpdated = GetTime();
}

MasternodeList::DockerUpdateMode MasternodeList::getCurUpdateMode()
{
    return m_updateMode;
}

void MasternodeList::updateEdgeStatus(int count)
{
    if(IsThreadRunning() && count){
        switchButton->SetSelected(true);
    }
    else{
        if(switchButton->IsSelected()){
            switchButton->SetSelected(false);
            ThreadEdgeStop();
        }
    }
}

bool MasternodeList::getVirtualIP(const QString& n2n_localip,const QString& n2n_netmask,QString& virtualIP)
{
    in_addr_t in_netmask = htonl(inet_addr(n2n_netmask.toStdString().c_str()));
    in_addr_t in_serviceip = htonl(inet_addr(n2n_localip.toStdString().c_str()));
    in_addr_t in_gateway = in_serviceip & in_netmask;
    in_addr_t ipend = (in_serviceip | ~in_netmask) & htonl(inet_addr("255.255.255.254"));
    in_addr_t ipstart = in_gateway | htonl(inet_addr("0.1.0.1"));

    boost::mt19937 gen(time(0));
    boost::uniform_int<> uni_dist(ipstart, ipend);
    boost::variate_generator<boost::mt19937&,boost::uniform_int<>>die(gen,uni_dist);
    in_addr ipaddr{};
    ipaddr.s_addr = ntohl(die());

#ifdef WIN32
    sockaddr_in in;
    memcpy(&in.sin_addr,&ipaddr.s_addr,INET_ADDRSTRLEN);
    virtualIP = QString(inet_ntoa(in.sin_addr));
#else
    char strip[INET_ADDRSTRLEN];
    virtualIP = QString(inet_ntop(AF_INET,&ipaddr.s_addr, strip, sizeof(strip)));
#endif
    return true;
}

void MasternodeList::slot_changeN2Nstatus(bool isSelected)
{
    switchButton->setEnabled(false);

    int curindex = ui->serviceTableWidget->currentRow();
    
    if(curindex <0 )
        return ;

    if(isSelected){

        std::string serviceid = ui->serviceTableWidget->item(curindex,0)->text().toStdString();
        Service service = dockercluster.dndata.mapDockerServiceLists[serviceid];

        std::vector<std::string> env = service.spec.taskTemplate.containerSpec.env;
        int count = env.size();
        QString n2n_name;
        QString n2n_localip;
        QString n2n_netmask = "255.0.0.0";
        QString n2n_SPIP;
        QString virtualIP;

        for (int i = 0; i < count; i++) {
            QString envStr = QString::fromStdString(env[i]);
            if (envStr.contains("N2N_NAME")) {
                n2n_name = envStr.split("=").at(1);
            } else if (envStr.contains("N2N_SERVERIP")) {
                n2n_localip = envStr.split("=").at(1);
            } else if (envStr.contains("N2N_NETMASK")) {
                n2n_netmask = envStr.split("=").at(1);
            } else if (envStr.contains("N2N_SNIP")) {
                n2n_SPIP = envStr.split("=").at(1);
            }
        }

        if(!getVirtualIP(n2n_localip,n2n_netmask,virtualIP)){

            switchButton->SetSelected(false);
            CMessageBox::information(this, tr("Edge option"),tr("Get remote ip error!"));
            return ;
        }

        bool startThreadFlag = ThreadEdgeStart(n2n_name.toStdString(),
                                               virtualIP.toStdString(),
                                               n2n_netmask.toStdString(),
                                               n2n_SPIP.toStdString(), MasternodeList::getEdgeRet);
                                               
        if(!startThreadFlag){
            ThreadEdgeStop();
        }
    }
    else{
        ThreadEdgeStop();
    }
}

void MasternodeList::getEdgeRet(bool flag)
{
    g_masternodeListPage->showEdgeRet(flag);
}

void MasternodeList::showEdgeRet(bool flag)
{
    switchButton->SetSelected(flag);
    switchButton->setEnabled(true);
}

void MasternodeList::gotoOrderDetailPage(int index)
{
    OrderDetail dlg(0);
    
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
    dlg.exec();
}

void MasternodeList::jumpToCheckOrder(int index)
{
    gotoOrderDetailPage(index);
}

void MasternodeList::jumpToCheckService(std::string ip_port)
{
    gotoDockerSerivcePage(ip_port);
}

void MasternodeList::jumpToCreateService(std::string ip,std::string txid)
{
    gotoCreateServicePage(ip,txid); 
}

void MasternodeList::slot_btn_refund()
{
    std::string txid,mnip,orderstatus;
    dockerorderView->getCurrentItemTxidAndmnIp(txid,mnip,orderstatus);

    deleteService(txid,mnip);
    //update order status
    dockerorderView->updateAllOperationBtn();
}

void MasternodeList::loadOrderData()
{
    dockerorderView->updateAllOperationBtn();
}

void MasternodeList::initDockerOrderView(const PlatformStyle *platformStyle)
{
    dockerorderView = new DockerOrderView(platformStyle, ui->tab_dockerorder);

    connect(dockerorderView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(dockerOrderViewdoubleClicked(QModelIndex)));
    connect(dockerorderView, SIGNAL(itemClicked(QModelIndex)), this, SLOT(dockerOrderViewitemClicked(QModelIndex)));
    connect(dockerorderView, SIGNAL(deleteService(std::string,std::string)), this, SLOT(deleteService(std::string,std::string)));
    connect(dockerorderView, SIGNAL(openServicePage(std::string)), this, SLOT(jumpToCheckService(std::string)));
    connect(dockerorderView, SIGNAL(gotoCreateServicePage(std::string,std::string)), this, SLOT(jumpToCreateService(std::string,std::string)));
    connect(ui->lineEdit_searchOrder,SIGNAL(textChanged(const QString&)),dockerorderView,SLOT(txidPrefix(const QString&)));
    ui->gridLayout_orderView->addWidget(dockerorderView);
}

void MasternodeList::dockerOrderViewdoubleClicked(QModelIndex index)
{

}

void MasternodeList::dockerOrderViewitemClicked(QModelIndex index)
{
    std::string txid,mnip,orderstatus;
    dockerorderView->getCurrentItemTxidAndmnIp(txid,mnip,orderstatus);
    if(orderstatus == "1"){
        ui->pushButton_refund->setEnabled(false);
        return ;
    }
    ui->pushButton_refund->setEnabled(true);
    // QTimer::singleShot(10000,this,SLOT(disenableButton()));
    startScanTimer(5000);
}

void MasternodeList::startScanTimer(int msec)
{
    if(m_scanTimer == NULL){
        m_scanTimer = new QTimer(this);
        connect(m_scanTimer,SIGNAL(timeout()),this,SLOT(timeoutToScanStatus()));
    }
    if(m_scanTimer->isActive()){
        m_scanTimer->stop();
    }
    m_scanTimer->start(msec);
}

void MasternodeList::timeoutToScanStatus()
{
    ui->pushButton_refund->setEnabled(false);
}

// void MasternodeList::askTransData(std::string txid)
// {
//     // AskForTransData
// }