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

extern MasternodeList* g_masternodeListPage;

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
    int row = index.row();
    const std::string& address_port = ui->tableWidgetMasternodes->item(row,0)->text().toStdString().c_str();

    m_curAddr_Port = address_port;
    const std::string& addr = DefaultReceiveAddress();
    CPubKey pubkey = pwalletMain->CreatePubKey(addr);
    if(dockercluster.SetConnectDockerAddress(address_port) && dockercluster.ProcessDockernodeConnections()){
        dockercluster.AskForDNData();
        ui->serviceTableWidget->setRowCount(0);
        clearDockerDetail();
        ui->tabWidget->setCurrentIndex(2);
        refreshServerList();
            return ;
    }
    CMessageBox::information(this, tr("Docker option"),tr("connect docker network failed!"));
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
    static int64_t nTimeMyListUpdated = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click

    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if(nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

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

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS seconds
    // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                            ? nTimeFilterUpdated - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS
                            : nTimeListUpdated - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
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

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        QTableWidgetItem *idItem = new QTableWidgetItem(id);

        ui->serviceTableWidget->insertRow(count);
        ui->serviceTableWidget->setItem(count, 0, nameItem);
        ui->serviceTableWidget->setItem(count, 1, idItem);
        count++;
    }
    if(!count)
        clearDockerDetail();
    return count;
}

void MasternodeList::loadServerDetail(QModelIndex index)
{
    QString key = ui->serviceTableWidget->item(index.row(),1)->text();
    Service service = dockercluster.mapDockerServiceLists[key.toStdString().c_str()];

    std::string name = service.spec.name;
    std::string address = service.spec.labels["com.massgrid.pubkey"];

    std::string image = service.spec.taskTemplate.containerSpec.image;
    std::string userName = service.spec.taskTemplate.containerSpec.user;

    map<std::string,Task> mapDockerTasklists = service.mapDockerTasklists;
    map<std::string,Task>::iterator iter = mapDockerTasklists.begin();


    LogPrintf("----->mapDockerTasklists size:%d \n",mapDockerTasklists.size());


    for(;iter != mapDockerTasklists.end();iter++){
        std::string id = iter->first;
        Task task = iter->second;
        QString name = QString::fromStdString(task.name);
        QString serviceID = QString::fromStdString(task.serviceID);
        int64_t slot = task.slot;

        int64_t nanoCPUs = task.spec.resources.limits.nanoCPUs;
        int64_t memoryBytes = task.spec.resources.limits.memoryBytes;

        std::string gpuName ;
        int64_t gpuCount = 0;

        int genericResourcesSize = task.spec.resources.reservations.genericResources.size();
        if(genericResourcesSize >0){
            gpuName = task.spec.resources.reservations.genericResources[0].discreateResourceSpec.kind;
            gpuCount = task.spec.resources.reservations.genericResources[0].discreateResourceSpec.value;
        }

        ui->label_taskName->setText(name);
        ui->label_cpuCount->setText(QString::number(nanoCPUs));
        ui->label_memoryBytes->setText(QString::number(memoryBytes));
        ui->label_GPUName->setText(QString::fromStdString(gpuName));
        ui->label_GPUCount->setText(QString::number(gpuCount));
    }
    
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
            if(size >= 3)
            ssh_pubkey =  envStr.split(" ").at(2);
        }
    }

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

void MasternodeList::slot_updateServiceBtn()
{
    loadServerList();
}

void MasternodeList::clearDockerDetail()
{
    ui->label_name->setText("");
    ui->label_n2n_localip->setText("");
    ui->label_image->setText("");
    ui->label_user->setText("");
    ui->label_n2n_serverip->setText("");
    ui->label_n2n_name->setText("");
    ui->label_n2n_localip->setText("");
    ui->label_ssh_pubkey->setText("");

    ui->label_taskName->setText("");
    ui->label_cpuCount->setText("");
    ui->label_memoryBytes->setText("");
    ui->label_GPUName->setText("");
    ui->label_GPUCount->setText("");
}

void MasternodeList::slot_createServiceBtn()
{
    AddDockerServiceDlg dlg;

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);
    dlg.setaddr_port(m_curAddr_Port);

    dlg.setWalletModel(walletModel);

    dlg.exec();
    QTimer::singleShot(2000,this,SLOT(refreshServerList()));
}

void MasternodeList::refreshServerList()
{
    static int countIndex = 0 ;
    int count = loadServerList();
    int currentIndex = ui->tabWidget->currentIndex();
    if(!count && (currentIndex == 2) && (countIndex <= 5) ){
        LogPrintf("refreshServerList:reload docker servicelist after 3.5s\n");
        QTimer::singleShot(3500,this,SLOT(refreshServerList()));
        countIndex ++ ;
        return ;
    }
    countIndex = 0;
}

void MasternodeList::slot_changeN2Nstatus(bool isSelected)
{
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
                                               n2n_SPIP.toStdString().c_str(),
                                               n2n_localip.toStdString().c_str(), g_masternodeListPage->getEdgeRet);
                                               
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
}