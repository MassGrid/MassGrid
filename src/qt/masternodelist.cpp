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

#include "cmessagebox.h"
#include "dockercluster.h"
#include "util.h"
#include "adddockerservicedlg.h"
#include "massgridgui.h"
#include "dockeredge.h"
#include "qswitchbutton.h"
#include "dockerserverman.h"
#include "masternode-sync.h"
#include <QLabel>
#include <QStringList>

#define DOCKER_AFTERCREATE_UPDATE_SECONDS 5
#define DOCKER_WHENNORMAL_UPDATE_SECONDS 600

extern MasternodeList* g_masternodeListPage;
extern CDockerServerman dockerServerman;

const char* strTaskStateTmp[]={"new", "allocated","pending","assigned", 
                        "accepted","preparing","ready","starting",
                        "running","complete","shutdown","failed",
                        "rejected","remove","orphaned"};
// QStringList strTaskStateTmpList;

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
    walletModel(0)
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

    ui->tableWidgetMasternodes->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetMasternodes->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetMasternodes->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetMasternodes->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetMasternodes->setColumnWidth(4, columnLastSeenWidth);

    ui->serviceTableWidget->setColumnWidth(0, 150);
    ui->serviceTableWidget->setColumnWidth(1, 150);

    ui->serviceTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->serviceTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->serviceTableWidget->horizontalHeader()->setStretchLastSection(true);

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *startAliasAction = new QAction(tr("Start alias"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->tableWidgetMasternodes, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDockerDetail(QModelIndex)));
    connect(ui->serviceTableWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(loadServerDetail(QModelIndex)));
    
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    connect(ui->updateServiceBtn, SIGNAL(clicked()), this, SLOT(slot_updateServiceBtn()));
    connect(ui->createServiceBtn, SIGNAL(clicked()), this, SLOT(slot_createServiceBtn()));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)),this,SLOT(slot_curTabPageChanged(int)));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
    updateNodeList();
    clearDockerDetail();
    ui->tabWidget->tabBar()->setTabEnabled(2,false);

    switchButton = new QSwitchButton(ui->frame_2);
    switchButton->SetSelected(false);
    switchButton->SetSize(120,32);
    switchButton->setEnabled(false);
    connect(switchButton,SIGNAL(clicked(bool)),this,SLOT(slot_changeN2Nstatus(bool)));

    // strTaskStateTmpList <<tr("new")<<tr("allocated")<<tr("pending")<<tr("assigned")
    //                     <<tr("accepted")<<tr("preparing")<<tr("ready")<< tr("starting")<< tr("running")
    //                     <<tr("complete")<<tr("shutdown")<<tr("failed")<<tr("rejected")<<tr("remove")<< tr("orphaned");

}

MasternodeList::~MasternodeList()
{
    delete ui;
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

    m_curAddr_Port = address_port;

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
        QTableWidgetItem *nodeCount = new QTableWidgetItem(QString::number(mn.lastPing.mdocker.nodeCount)+"/"+QString::number(mn.lastPing.mdocker.activeNodeCount));
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
        ui->tableWidgetMasternodes->setItem(0, 0, addressItem);
        ui->tableWidgetMasternodes->setItem(0, 1, protocolItem);
        ui->tableWidgetMasternodes->setItem(0, 2, statusItem);
        ui->tableWidgetMasternodes->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetMasternodes->setItem(0, 4, lastSeenItem);
        ui->tableWidgetMasternodes->setItem(0, 5, pubkeyItem);
        ui->tableWidgetMasternodes->setItem(0, 6, nodeCount);
        ui->tableWidgetMasternodes->setItem(0, 7, joinToken);                
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
        // switchButton->SetSelected(false);
        setCurUpdateMode(DockerUpdateMode::WhenNormal);
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

    if(nSecondsTillUpdate > 0 && !fForce) return;
    m_nTimeDockerListUpdated = GetTime();

    // setCurUpdateMode(DockerUpdateMode::WhenNormal);

    if(getCurUpdateMode() == DockerUpdateMode::AfterCreate)
            askDNData();
        // refreshServerList();
    else if(DockerUpdateMode::WhenNormal)
        updateServiceList();
}

int MasternodeList::loadServerList()
{
    ui->serviceTableWidget->setRowCount(0);

    std::map<std::string,Service> serverlist = dockercluster.mapDockerServiceLists;
    std::map<std::string,Service>::iterator iter = serverlist.begin();

    int count = 0;
    for(;iter != serverlist.end();iter++){
        QString id = QString::fromStdString(iter->first);
        Service service = serverlist[iter->first];
        QString name = QString::fromStdString(service.spec.name);
        map<std::string,Task> mapDockerTasklists = service.mapDockerTasklists;

        int taskStatus = -1;
        QString taskStatusStr = tr("Waiting");
        if(mapDockerTasklists.size() > 0){
            Task task = mapDockerTasklists.begin()->second;
            taskStatus = task.status.state;
            taskStatusStr = taskStatus == 8 ? tr("Create completed") : tr("Creating...");
            //QString::fromStdString(strTaskStateTmp[taskStatus]);
        }

        QLabel *label = new QLabel(ui->serviceTableWidget);

        label->setText(taskStatusStr);
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        label->setStyleSheet(taskStatus == 8 ? "color:green;" : "color:red;");

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        QTableWidgetItem *idItem = new QTableWidgetItem(id);

        ui->serviceTableWidget->insertRow(count);
        ui->serviceTableWidget->setItem(count, 0, nameItem);
        ui->serviceTableWidget->setItem(count, 1, idItem);
        // ui->serviceTableWidget->setItem(count, 2, statusItem);
        ui->serviceTableWidget->setCellWidget(count,2,label);
        count++;
    }
    dockerServerman.setDNDataStatus(CDockerServerman::Free);
    return count;
}

void MasternodeList::loadServerDetail(QModelIndex index)
{
    QString key = ui->serviceTableWidget->item(index.row(),1)->text();
    loadDockerDetail(key.toStdString());
}

void MasternodeList::loadDockerDetail(const std::string & key)
{
    Service service = dockercluster.mapDockerServiceLists[key.c_str()];
    map<std::string,Task> mapDockerTasklists = service.mapDockerTasklists;

    // LogPrintf("loadDockerDetail service:%s\n",service.ToJsonString());

    int taskStatus = -1;
    updateTaskDetail(mapDockerTasklists,taskStatus);

    DockerUpdateMode mode = taskStatus == 8 ? DockerUpdateMode::WhenNormal : DockerUpdateMode::AfterCreate;

    updateServiceDetail(service);
    setCurUpdateMode(mode);
}

void MasternodeList::updateServiceDetail(Service& service)
{    
    uint64_t createdAt = service.createdAt;

    std::string name = service.spec.name;
    std::string address = service.spec.labels["com.massgrid.pubkey"];

    std::string image = service.spec.taskTemplate.containerSpec.image;
    std::string userName = service.spec.taskTemplate.containerSpec.user;

    std::vector<std::string> env = service.spec.taskTemplate.containerSpec.env;
    int count = env.size();
    QString n2n_name;
    QString n2n_localip;
    QString n2n_SPIP;
    QString ssh_pubkey;
    for(int i=0;i<count;i++){
        QString envStr = QString::fromStdString(env[i]);
        if(envStr.contains("N2N_NAME")){
            n2n_name = envStr.split("=").at(1);
        }
        else if(envStr.contains("N2N_SERVERIP")){
            n2n_localip = envStr.split("=").at(1);
        }
        else if(envStr.contains("N2N_SNIP")){
            n2n_SPIP = envStr.split("=").at(1);
        }
        else if(envStr.contains("SSH_PUBKEY")){
            int size = envStr.split(" ").size();
            if(size >= 2)
            ssh_pubkey =  envStr.split(" ").at(1).mid(0,10);
        }
    }

    ui->label_serviceTimeout->setText(QDateTime::fromTime_t(createdAt).addSecs(14400).toString("yyyy-MM-dd hh:mm:ss t"));
    ui->label_n2n_serverip->setText(n2n_SPIP);
    ui->label_n2n_name->setText(n2n_name);
    ui->label_n2n_localip->setText(n2n_localip);
    ui->label_ssh_pubkey->setText(ssh_pubkey);

    ui->label_name->setText(QString::fromStdString(name));
    ui->label_image->setText(QString::fromStdString(image));
    ui->label_user->setText(QString::fromStdString(userName));

    if( (!n2n_localip.isEmpty()) && (!n2n_SPIP.isEmpty()) ){
        switchButton->setEnabled(true);
    }
}

void MasternodeList::updateTaskDetail(std::map<std::string,Task> &mapDockerTasklists,int& taskStatus)
{
    map<std::string,Task>::iterator iter = mapDockerTasklists.begin();

    LogPrintf("mapDockerTasklists size:%d \n",mapDockerTasklists.size());

    for(;iter != mapDockerTasklists.end();iter++){
        std::string id = iter->first;
        Task task = iter->second;
        QString name = QString::fromStdString(task.name);
        QString serviceID = QString::fromStdString(task.serviceID);
        int64_t slot = task.slot;

        //std::string
        // int taskstatus = -1;
        taskStatus = task.status.state;
        QString taskStatusStr = QString::fromStdString(strTaskStateTmp[taskStatus]);

        QString taskErr = QString::fromStdString(task.status.err);

        int64_t nanoCPUs = task.spec.resources.limits.nanoCPUs;
        int64_t memoryBytes = task.spec.resources.limits.memoryBytes;

        std::string gpuName ;
        int64_t gpuCount = 0;

        int genericResourcesSize = task.spec.resources.reservations.genericResources.size();
        if(genericResourcesSize >0){
            gpuName = task.spec.resources.reservations.genericResources[0].discreateResourceSpec.kind;
            gpuCount = task.spec.resources.reservations.genericResources[0].discreateResourceSpec.value;
        }

        QString taskRuntime = QString::fromStdString(task.spec.runtime);

        ui->label_taskName->setText(QString::fromStdString(id));
        ui->label_cpuCount->setText(QString::number(nanoCPUs/DOCKER_CPU_UNIT));
        ui->label_memoryBytes->setText(QString::number(memoryBytes/DOCKER_MEMORY_UNIT));
        ui->label_GPUName->setText(QString::fromStdString(gpuName));
        ui->label_GPUCount->setText(QString::number(gpuCount));

        ui->label_taskStatus->setText(taskStatusStr);

        if(taskStatus == 8){
            ui->label_16->setStyleSheet("color:green;");
            ui->label_taskStatus->setStyleSheet("color:green;");
        }
        else if(taskStatus != -1){
            ui->label_16->setStyleSheet("color:red;");
            ui->label_taskStatus->setStyleSheet("color:red;");
        }
        else{
            ui->label_16->setStyleSheet("color:black;");
            ui->label_taskStatus->setStyleSheet("color:black;");
        }
        
        if(!taskErr.isEmpty()){
            ui->label_17->setVisible(true);
            ui->textEdit_taskErr->setVisible(true);
            ui->textEdit_taskErr->setText(taskErr);
        }
    }
}

void MasternodeList::slot_updateServiceBtn()
{
    setCurUpdateMode(DockerUpdateMode::WhenNormal);
    updateDockerList(true);
}

void MasternodeList::updateServiceList()
{
    const std::string& addr = DefaultReceiveAddress();
    CPubKey pubkey = pwalletMain->CreatePubKey(addr);

    clearDockerDetail();
    
    if(dockercluster.SetConnectDockerAddress(m_curAddr_Port) && dockercluster.ProcessDockernodeConnections()){
        askDNData();
    }
    else{
        CMessageBox::information(this, tr("Docker option"),tr("connect docker network failed!"));
        // reback to masternode list page
    }
}

void MasternodeList::askDNData()
{
    dockercluster.AskForDNData();
    ui->tabWidget->setCurrentIndex(2);
    ui->createServiceBtn->setEnabled(false);
    refreshServerList();
}

void MasternodeList::clearDockerDetail()
{
    ui->label_name->setText("");
    ui->label_n2n_localip->setText("");
    ui->label_image->setText("");
    ui->label_user->setText("");
    ui->label_serviceTimeout->setText("");
    ui->label_n2n_serverip->setText("");
    ui->label_n2n_name->setText("");
    ui->label_n2n_localip->setText("");
    ui->label_ssh_pubkey->setText("");

    ui->label_taskName->setText("");
    ui->label_cpuCount->setText("");
    ui->label_memoryBytes->setText("");
    ui->label_GPUName->setText("");
    ui->label_GPUCount->setText("");
    ui->label_taskStatus->setText("");
    ui->textEdit_taskErr->setText("");
    ui->label_17->setVisible(false);
    ui->textEdit_taskErr->setVisible(false);
    ui->serviceTableWidget->setRowCount(0);
}

void MasternodeList::slot_createServiceBtn()
{
    AddDockerServiceDlg dlg;

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
    dlg.setaddr_port(m_curAddr_Port);

    dlg.setWalletModel(walletModel);

    if(dlg.exec() == QDialog::Accepted){
        ui->createServiceBtn->setEnabled(false);
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
            ui->createServiceBtn->setEnabled(true);
            setCurUpdateMode(DockerUpdateMode::WhenNormal);
        }
        else{
            ui->createServiceBtn->setEnabled(false);
            QString key = ui->serviceTableWidget->item(0,1)->text();
            loadDockerDetail(key.toStdString());
        }
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

void MasternodeList::slot_changeN2Nstatus(bool isSelected)
{
    switchButton->setEnabled(false);
    if(isSelected){
        QString n2n_SPIP = ui->label_n2n_serverip->text();
        QString n2n_localip = ui->label_n2n_localip->text();
        QString n2n_name = ui->label_n2n_name->text();
        QStringList list = n2n_localip.split(".");

        if(list.size() != 4){
            switchButton->SetSelected(false);
            CMessageBox::information(this, tr("Edge option"),tr("Get remote ip error!"));
            return ;
        }

        int ipNum = QString(list.last()).toInt();
        ipNum ++ ;
        n2n_localip = QString("%1.%2.%3.%4").arg(list.at(0)).arg(list.at(1)).arg(list.at(2)).arg(QString::number(ipNum));

        bool startThreadFlag = ThreadEdgeStart(n2n_name.toStdString().c_str(),
                                               n2n_localip.toStdString().c_str(),
                                               n2n_SPIP.toStdString().c_str(), g_masternodeListPage->getEdgeRet);
                                               
        if(!startThreadFlag)
            ThreadEdgeStop();
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