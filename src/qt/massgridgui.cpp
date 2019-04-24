// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "massgridgui.h"

#include "massgridunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "modaloverlay.h"
#include "networkstyle.h"
#include "notificator.h"
#include "openuridialog.h"
#include "optionsdialog.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "rpcconsole.h"
#include "utilitydialog.h"
#include "mainwintitle.h"
#include "cupdatethread.h"
#include "cprogressdialog.h"
#include "clientversion.h"
#include "miner.h"
#ifdef ENABLE_WALLET
#include "walletframe.h"
#include "walletmodel.h"
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QMessageBox>

#include "chainparams.h"
#include "init.h"
#include "ui_interface.h"
#include "util.h"
#include "masternode-sync.h"
#include "cmessagebox.h"
#include "privkeymgr.h"
// #include "rpcserver.h"
#include "addresstablemodel.h"
#include "masternodelist.h"

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QListWidget>
#include <QMenuBar>
#include "cmessagebox.h"
#include <QMimeData>
#include <QProgressDialog>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QEventLoop>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QSizeGrip>
#include <QScreen>

#if QT_VERSION < 0x050000
#include <QTextDocument>
#include <QUrl>
#else
#include <QUrlQuery>
#endif

QPoint m_winPos;
QSize m_winSize;

const std::string MassGridGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

const QString MassGridGUI::DEFAULT_WALLET = "~Default";

MassGridGUI::MassGridGUI(const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    clientModel(0),
    walletFrame(0),
    unitDisplayControl(0),
    labelEncryptionIcon(0),
    labelWalletHDStatusIcon(0),
    labelConnectionsIcon(0),
    labelBlocksIcon(0),
    progressBarLabel(0),
    progressBar(0),
    progressDialog(0),
    appMenuBar(0),
    overviewAction(0),
    historyAction(0),
    masternodeAction(0),
    quitAction(0),
    sendCoinsAction(0),
    sendCoinsMenuAction(0),
    usedSendingAddressesAction(0),
    usedReceivingAddressesAction(0),
    signMessageAction(0),
    verifyMessageAction(0),
    aboutAction(0),
    receiveCoinsAction(0),
    receiveCoinsMenuAction(0),
    optionsAction(0),
    toggleHideAction(0),
    encryptWalletAction(0),
    backupWalletAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    openRPCConsoleAction(0),
    openAction(0),
    showHelpMessageAction(0),
    showPrivateSendHelpAction(0),
    importPrivKeyAction(0),
    dumpPrivKeyAction(0),
    trayIcon(0),
    trayIconMenu(0),
    dockIconMenu(0),
    notificator(0),
    rpcConsole(0),
    helpMessageDialog(0),
    modalOverlay(0),
    prevBlocks(0),
    spinnerFrame(0),
    m_mainTitle(0),
    m_updateClientThread(0),
    platformStyle(platformStyle)
{
    QString windowTitle = tr("MassGrid") + " - ";
#ifdef ENABLE_WALLET
    /* if compiled with wallet support, -disablewallet can still disable the wallet */
    enableWallet = !GetBoolArg("-disablewallet", false);
#else
    enableWallet = false;
#endif // ENABLE_WALLET
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }
    QString userWindowTitle = QString::fromStdString(GetArg("-windowtitle", ""));
    if(!userWindowTitle.isEmpty()) windowTitle += " - " + userWindowTitle;
    windowTitle += " " + networkStyle->getTitleAddText();
#ifndef Q_OS_MAC
    QApplication::setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
#else
    MacDockIconHandler::instance()->setIcon(networkStyle->getAppIcon());
#endif
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC) && QT_VERSION < 0x050000
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    rpcConsole = new RPCConsole(platformStyle, 0);
    helpMessageDialog = new HelpMessageDialog(0, HelpMessageDialog::cmdline);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame*/
        // walletFrame = new WalletFrame(platformStyle, this);
        createMainWin(platformStyle);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create system tray icon and notification
    createTrayIcon(networkStyle);

    // Create application menu bar
    if(m_mainTitle)
        createMenuBar();

    // Create the toolbars
    // createToolBars();


    // Create status bar
    // statusBar();

    // this->setStatusBar(NULL);

/*----------------------
    // Create application menu bar
    if(m_mainTitle)
        createMenuBar();
    // Create the toolbars 
    // createToolBars(); 

    // Create status bar
    // statusBar();
*/



    // Disable size grip because it looks ugly and nobody needs it
    // statusBar()->setSizeGripEnabled(false);

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    unitDisplayControl = new UnitDisplayStatusBarControl(platformStyle);
    labelEncryptionIcon = new QLabel();
    labelWalletHDStatusIcon = new QLabel();
    labelConnectionsIcon = new GUIUtil::ClickableLabel();

    labelBlocksIcon = new GUIUtil::ClickableLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelEncryptionIcon);
        frameBlocksLayout->addWidget(labelWalletHDStatusIcon);
    }
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    statusFrame->setObjectName("statusFrame");

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(true);
    progressBarLabel->setMaximumWidth(150);
    progressBarLabel->setStyleSheet("color:rgb(255,255,255);");
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(true);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: rgb(49, 61, 64); border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #00CCFF, stop: 1 #33CCFF); border-radius: 7px; margin: 0px; }");
    }

    progressBar->setMinimumSize(500,30);

    QHBoxLayout *statusFrameLayout = new QHBoxLayout(statusFrame);
    statusFrame->setLayout(statusFrameLayout);

    statusFrameLayout->setContentsMargins(9,0,0,0);
    statusFrameLayout->setSpacing(0);

    QSpacerItem* spacerItem = new QSpacerItem(1200,20);

    QLabel * tmpSpacer = new QLabel(this);
    statusFrameLayout->addWidget(progressBarLabel);
    statusFrameLayout->addWidget(progressBar);
    // statusFrameLayout->addSpacerItem(spacerItem);
    statusFrameLayout->addWidget(tmpSpacer);
    statusFrameLayout->addWidget(frameBlocks);

    QFrame* tmpFrame = new QFrame(statusFrame);
    tmpFrame->setFixedSize(15,statusFrame->height());
    statusFrameLayout->addWidget(tmpFrame);

    sizeGrip = new QSizeGrip(tmpFrame);

#ifdef Q_OS_WIN
    sizeGrip->move(2,25);
#else
    sizeGrip->move(-3,10);
#endif

    // statusBar()->addWidget(progressBarLabel);
    // statusBar()->addWidget(progressBar);
    // statusBar()->addPermanentWidget(frameBlocks);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    // Jump to peers tab by clicking on connections icon
    connect(labelConnectionsIcon, SIGNAL(clicked(QPoint)), this, SLOT(showPeers()));

    modalOverlay = new ModalOverlay(this->centralWidget());
#ifdef ENABLE_WALLET
    if(enableWallet) {
        connect(walletFrame, SIGNAL(requestedSyncWarningInfo()), this, SLOT(showModalOverlay()));
        connect(labelBlocksIcon, SIGNAL(clicked(QPoint)), this, SLOT(showModalOverlay()));
        connect(progressBar, SIGNAL(clicked(QPoint)), this, SLOT(showModalOverlay()));
    }
#endif
    // resize(850,650);

    QScreen* screen = qApp->primaryScreen();
    int dpi = screen->logicalDotsPerInch()/72;

    // int winWidth =
    // 系数 =  width / height;
    // if width/height > 系数 : width = height*系数
    // int minwidth = 850;
    // int minheight = 550;
    // qreal coefficient = minwidth/minheight;

    GUIUtil::restoreWindowGeometry("nWindow", QSize(850*dpi, 550*dpi), this);

    m_winPos = this->pos();
    m_winSize = this->size();
}

MassGridGUI::~MassGridGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

    if(m_updateClientThread != 0){
        QEventLoop loop;
        connect(m_updateClientThread,SIGNAL(threadStoped()),&loop,SLOT(quit()));
        m_updateClientThread->stopThread();
        loop.exec();
        m_updateClientThread->quit();
        m_updateClientThread->wait();
        delete m_updateClientThread;
    }
    delete rpcConsole;
}


void MassGridGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
#ifdef Q_OS_MAC
    overviewAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
#else
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
#endif
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a MassGrid address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
#ifdef Q_OS_MAC
    sendCoinsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_2));
#else
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
#endif
    tabGroup->addAction(sendCoinsAction);

    sendCoinsMenuAction = new QAction(QIcon(":/icons/send"), sendCoinsAction->text(), this);
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and massgrid: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
#ifdef Q_OS_MAC
    receiveCoinsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_3));
#else
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
#endif
    tabGroup->addAction(receiveCoinsAction);

    receiveCoinsMenuAction = new QAction(QIcon(":/icons/receiving_addresses"), receiveCoinsAction->text(), this);
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
#ifdef Q_OS_MAC
    historyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_4));
#else
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
#endif
    tabGroup->addAction(historyAction);

#ifdef ENABLE_WALLET
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction = new QAction(QIcon(":/icons/masternodes"), tr("&Masternodes"), this);
        masternodeAction->setStatusTip(tr("Browse masternodes"));
        masternodeAction->setToolTip(masternodeAction->statusTip());
        masternodeAction->setCheckable(true);
#ifdef Q_OS_MAC
        masternodeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_5));
#else
        masternodeAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
#endif
        tabGroup->addAction(masternodeAction);
        connect(masternodeAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(masternodeAction, SIGNAL(triggered()), this, SLOT(gotoMasternodePage()));
    }

    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
#endif // ENABLE_WALLET

    quitAction = new QAction(QIcon(":/res/pic/menuicon/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/res/pic/menuicon/about"), tr("&About MassGrid"), this);
    aboutAction->setStatusTip(tr("Show information about MassGrid"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(QIcon(":/res/pic/menuicon/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/res/pic/menuicon/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for MassGrid"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    toggleHideAction = new QAction(QIcon(":/res/pic/menuicon/about"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(QIcon(":/res/pic/menuicon/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/res/pic/menuicon/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/res/pic/menuicon/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(tr("&Lock Wallet"), this);
    signMessageAction = new QAction(QIcon(":/res/pic/menuicon/verifyMessage"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your MassGrid addresses to prove you own them"));
    verifyMessageAction = new QAction(QIcon(":/res/pic/menuicon/verifyMessage"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified MassGrid addresses"));

    openInfoAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("&Information"), this);
    openInfoAction->setStatusTip(tr("Show diagnostic information"));
    openRPCConsoleAction = new QAction(QIcon(":/res/pic/menuicon/debugwindow"), tr("&Debug console"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging console"));
    openGraphAction = new QAction(QIcon(":/res/pic/menuicon/connect_4"), tr("&Network Monitor"), this);
    openGraphAction->setStatusTip(tr("Show network monitor"));
    openPeersAction = new QAction(QIcon(":/res/pic/menuicon/connect_4"), tr("&Peers list"), this);
    openPeersAction->setStatusTip(tr("Show peers info"));
    openRepairAction = new QAction(QIcon(":/res/pic/menuicon/options"), tr("Wallet &Repair"), this);
    openRepairAction->setStatusTip(tr("Show wallet repair options"));
    openConfEditorAction = new QAction(QIcon(":/res/pic/menuicon/edit"), tr("Open Wallet &Configuration File"), this);
    openConfEditorAction->setStatusTip(tr("Open configuration file"));
    openMNConfEditorAction = new QAction(QIcon(":/res/pic/menuicon/edit"), tr("Open &Masternode Configuration File"), this);
    openMNConfEditorAction->setStatusTip(tr("Open Masternode configuration file"));    
    showBackupsAction = new QAction(QIcon(":/res/pic/menuicon/filesave"), tr("Show Automatic &Backups"), this);
    showBackupsAction->setStatusTip(tr("Show automatically created wallet backups"));
    // initially disable the debug window menu items
    openInfoAction->setEnabled(false);
    openRPCConsoleAction->setEnabled(false);
    openGraphAction->setEnabled(false);
    openPeersAction->setEnabled(false);
    openRepairAction->setEnabled(false);

    usedSendingAddressesAction = new QAction(QIcon(":/res/pic/menuicon/address-book"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(QIcon(":/res/pic/menuicon/address-book"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a massgrid: URI or payment request"));

    showHelpMessageAction = new QAction(QIcon(":/res/pic/menuicon/command"), tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the MassGrid help message to get a list with possible MassGrid command-line options"));

    importPrivKeyAction = new QAction(QIcon(":/res/pic/menuicon/importprivkey.png"), tr("&Import privkey"), this);
    importPrivKeyAction->setStatusTip(tr("import the private key."));
    showPrivateSendHelpAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("&PrivateSend information"), this);
    dumpPrivKeyAction = new QAction(QIcon(":/res/pic/menuicon/dumpprivkey.png"), tr("&Dump privkey"), this);
    dumpPrivKeyAction->setStatusTip(tr("dump out the private key."));
    showPrivateSendHelpAction->setMenuRole(QAction::NoRole);
    showPrivateSendHelpAction->setStatusTip(tr("Show the PrivateSend basic information"));
    
    softUpdateAction = new QAction(QIcon(":/res/pic/menuicon/softupdate.png"), tr("&Soft Update"), this);
    softUpdateAction->setStatusTip(tr("Soft Update"));


    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
    connect(showPrivateSendHelpAction, SIGNAL(triggered()), this, SLOT(showPrivateSendHelpClicked()));

    // Jump directly to tabs in RPC-console
    connect(openInfoAction, SIGNAL(triggered()), this, SLOT(showInfo()));
    connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showConsole()));
    connect(openGraphAction, SIGNAL(triggered()), this, SLOT(showGraph()));
    connect(openPeersAction, SIGNAL(triggered()), this, SLOT(showPeers()));
    connect(openRepairAction, SIGNAL(triggered()), this, SLOT(showRepair()));

    // Open configs and backup folder from menu
    connect(openConfEditorAction, SIGNAL(triggered()), this, SLOT(showConfEditor()));
    connect(openMNConfEditorAction, SIGNAL(triggered()), this, SLOT(showMNConfEditor()));
    connect(showBackupsAction, SIGNAL(triggered()), this, SLOT(showBackups()));

    // Get restart command-line parameters and handle restart
    connect(rpcConsole, SIGNAL(handleRestart(QStringList)), this, SLOT(handleRestart(QStringList)));
    
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(softUpdateAction, SIGNAL(triggered()), this, SLOT(checkoutUpdateClient()));
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
        connect(unlockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(unlockWallet()));
        connect(lockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(lockWallet()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));
        connect(importPrivKeyAction, SIGNAL(triggered()), this, SLOT(importPrivkey()));
        connect(dumpPrivKeyAction, SIGNAL(triggered()), this, SLOT(dumpPrivkey()));
    }
#endif // ENABLE_WALLET

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I), this, SLOT(showInfo()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this, SLOT(showConsole()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G), this, SLOT(showGraph()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P), this, SLOT(showPeers()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R), this, SLOT(showRepair()));
}



// void MassGridGUI::createMenuBar()
// {
// #ifdef Q_OS_MAC
//     // Create a decoupled menu bar on Mac which stays even if the window is closed
//     appMenuBar = new QMenuBar();
// #else
//     // Get the main window's menu bar on other platforms
//     appMenuBar = menuBar();
// #endif

//     // Configure the menus
//     // QMenu *file = appMenuBar->addMenu(tr("&File"));
//     QMenu* file = m_mainTitle->fileMenu(tr("&File"));
//     if(walletFrame)
//     {
//         file->addAction(openAction);
//         file->addAction(backupWalletAction);
//         file->addAction(signMessageAction);
//         file->addAction(verifyMessageAction);
//         file->addSeparator();
//         file->addAction(usedSendingAddressesAction);
//         file->addAction(usedReceivingAddressesAction);
//         file->addSeparator();
//     }
//     file->addAction(quitAction);

//     QMenu *settings = m_mainTitle->settingsMenu(tr("&Settings"));
//     if(walletFrame)
//     {
//         settings->addAction(encryptWalletAction);
//         settings->addAction(changePassphraseAction);
//         settings->addAction(unlockWalletAction);
//         settings->addAction(lockWalletAction);
//         settings->addSeparator();
//     }
//     settings->addAction(optionsAction);

//     if(walletFrame)
//     {
//         QMenu *tools = m_mainTitle->helpMenu(tr("&Tools"));
//         tools->addAction(openInfoAction);
//         tools->addAction(openRPCConsoleAction);
//         tools->addAction(openGraphAction);
//         tools->addAction(openPeersAction);
//         tools->addAction(openRepairAction);
//         tools->addSeparator();
//         tools->addAction(openConfEditorAction);
//         tools->addAction(openMNConfEditorAction);
//         tools->addAction(showBackupsAction);
//     }

//     QMenu *help = appMenuBar->addMenu(tr("&Help"));
//     help->addAction(showHelpMessageAction);
//     help->addAction(showPrivateSendHelpAction);
//     help->addSeparator();
//     help->addAction(aboutAction);
//     help->addAction(aboutQtAction);
// }

void MassGridGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    // QMenu *file = appMenuBar->addMenu(tr("&File"));
    QMenu* file = m_mainTitle->fileMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addAction(importPrivKeyAction);
        file->addAction(dumpPrivKeyAction);
        file->addAction(softUpdateAction);
        //file->addSeparator();
    }
    //file->addAction(quitAction);

    QMenu *settings = m_mainTitle->settingsMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(lockWalletAction);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    if(walletFrame)
    {
        QMenu *tools = m_mainTitle->helpMenu(tr("&Tools"));
        // tools->addAction(openInfoAction);
        tools->addAction(openRPCConsoleAction);
        // tools->addAction(openGraphAction);
        // tools->addAction(openPeersAction);
        // tools->addAction(openRepairAction);
        tools->addAction(showHelpMessageAction);
        tools->addSeparator();
        tools->addAction(openMNConfEditorAction);
        tools->addAction(openConfEditorAction);
        tools->addAction(showBackupsAction);
        tools->addAction(aboutAction);
        tools->addAction(aboutQtAction);
    }

    // QMenu *help = appMenuBar->addMenu(tr("&Help"));
    // help->addAction(showHelpMessageAction);
    // help->addAction(showPrivateSendHelpAction);
    // help->addSeparator();
    // help->addAction(aboutAction);
    // help->addAction(aboutQtAction);
}

// void MassGridGUI::createToolBars()
// {
// #ifdef ENABLE_WALLET
//     if(walletFrame)
//     {
//         QToolBar *toolbar = new QToolBar(tr("Tabs toolbar"));
//         toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//         toolbar->addAction(overviewAction);
//         toolbar->addAction(sendCoinsAction);
//         toolbar->addAction(receiveCoinsAction);
//         toolbar->addAction(historyAction);
//         QSettings settings;
//         if (settings.value("fShowMasternodesTab").toBool())
//         {
//             toolbar->addAction(masternodeAction);
//         }
//         toolbar->setMovable(false); // remove unused icon in upper left corner
//         overviewAction->setChecked(true);

//         /** Create additional container for toolbar and walletFrame and make it the central widget.
//             This is a workaround mostly for toolbar styling on Mac OS but should work fine for every other OSes too.
//         */
//         QVBoxLayout *layout = new QVBoxLayout;
//         // layout->addWidget(toolbar);
//         layout->addWidget(walletFrame);
//         layout->setSpacing(0);
//         layout->setContentsMargins(QMargins());
//         QWidget *containerWidget = new QWidget();
//         containerWidget->setLayout(layout);
//         setCentralWidget(containerWidget);
//     }
// #endif // ENABLE_WALLET
// }

void MassGridGUI::createBackgroundWin()
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    QGridLayout* layout = new QGridLayout(this);
    backgroudlayout = new QGridLayout(this);
    this->setLayout(layout);

    layout->setContentsMargins(0,0,0,0);
    centerWin = new QWidget(this);
    #ifndef Q_OS_MAC
        centerWin->setObjectName("centerWin");
    #endif

    layout->addWidget(centerWin);
    
    centerWin->setLayout(backgroudlayout);

    mainFrame = new QFrame(this);
    mainFrame->setObjectName("mainFrame");

    mainFrame->setStyleSheet("QFrame#mainFrame{\n   background-color: rgb(255, 255, 255);\n}");
    mainFrame->setMinimumSize(850,650);

    backgroudlayout->addWidget(mainFrame);
    backgroudlayout->setContentsMargins(40,40,40,40);
    centerWin->setLayout(backgroudlayout);

    this->setCentralWidget(centerWin);
    this->setObjectName("MassGridGUI");
}

void MassGridGUI::createMainWin(const PlatformStyle *platformStyle)
{
    createBackgroundWin();

    m_mainTitle = new MainwinTitle();
    // m_mainTitle->setTitle(networkStyle->getTitleAddText());

    connect(m_mainTitle,SIGNAL(sgl_close()),this,SLOT(close()));
    connect(m_mainTitle,SIGNAL(sgl_showMin()),this,SLOT(showMinimized()));
    connect(m_mainTitle,SIGNAL(sgl_showMax()),this,SLOT(showMaxWin())); 

    connect(this,SIGNAL(updateBalance(QString,QString,QString,bool,bool,QString)),m_mainTitle,SLOT(updateBalance(QString ,QString ,QString,bool,bool,QString)));

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.

    connect(m_mainTitle,SIGNAL(sgl_showOverview()),this,SLOT(showNormalIfMinimized()));
    connect(m_mainTitle,SIGNAL(sgl_showOverview()),this,SLOT(gotoOverviewPage()));

    connect(m_mainTitle,SIGNAL(sgl_showSendPage()),this,SLOT(showNormalIfMinimized()));
    connect(m_mainTitle,SIGNAL(sgl_showSendPage()),this,SLOT(gotoSendCoinsPage()));  

    connect(m_mainTitle,SIGNAL(sgl_showExchangePage()),this,SLOT(showNormalIfMinimized()));
    connect(m_mainTitle,SIGNAL(sgl_showExchangePage()),this,SLOT(gotoHistoryPage()));

    connect(m_mainTitle,SIGNAL(sgl_showMasternodePage()),this,SLOT(showNormalIfMinimized()));
    connect(m_mainTitle,SIGNAL(sgl_showMasternodePage()),this,SLOT(gotoMasternodePage()));
    

#endif // ENABLE_WALLET

    walletFrame = new WalletFrame(platformStyle,this);

    statusFrame = new QFrame(this);
    // statusFrame->setStyleSheet("background-color:rgb(255,255,255,0);");
    // statusFrame->setMaximumSize(100000,40);
    statusFrame->setMinimumSize(800,40);
    statusFrame->setMaximumHeight(40);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget((m_mainTitle));
    layout->addWidget(walletFrame);
    layout->addWidget(statusFrame);

    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    // win->setLayout(layout);
    // this->setCentralWidget(win);
    // this->setLayout(layout);
    layout->setStretch(0,5);
    layout->setStretch(1,10);
    layout->setStretch(2,1);
    mainFrame->setLayout(layout);
    // this->setCentralWidget(mainFrame);

    // mainFrame->setObjectName("mainFrame");
    // mainFrame->setStyleSheet("QFrame#mainFrame{background-color: rgb(255, 0, 0);}");
}

void MassGridGUI::updateAddr(WalletModel *walletModel)
{
    AddressTableModel *addrmodel = walletModel->getAddressTableModel();
    if(!addrmodel)
        return ;
    m_mainTitle->setModel(walletModel);
    m_mainTitle->loadRow(0);
}

void MassGridGUI::showMaxWin()
{
    if(this->isMaximized()){
        #ifndef Q_OS_MAC
            if(backgroudlayout)
                backgroudlayout->setContentsMargins(40,40,40,40);
        #endif
        this->showNormal();
    }
    else{
        if(backgroudlayout)
            backgroudlayout->setContentsMargins(0,0,0,0);
        this->showMaximized();
    }
}  

QPoint MassGridGUI::winPos()
{
    return m_winPos;
}

QSize MassGridGUI::winSize()
{
    return m_winSize;
}

void MassGridGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        if (trayIcon) {
            // // do so only if trayIcon is already set
            // trayIconMenu = new QMenu(this);
            // trayIcon->setContextMenu(trayIconMenu);
            // createIconMenu(trayIconMenu);

#ifdef Q_OS_MAC
            createTrayIconMenu();
#endif


#ifndef Q_OS_MAC
            // Show main window on tray icon click
            // Note: ignore this on Mac - this is not the way tray should work there
            connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                    this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
            // Note: On Mac, the dock icon is also used to provide menu functionality
            // similar to one for tray icon
            MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
            dockIconHandler->setMainWindow((QMainWindow *)this);
            dockIconMenu = dockIconHandler->dockMenu();
 
            createIconMenu(dockIconMenu);
#endif
        }

        // Keep up to date with client
        updateNetworkState();
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));
        connect(clientModel, SIGNAL(networkActiveChanged(bool)), this, SLOT(setNetworkActive(bool)));

        modalOverlay->setKnownBestHeight(clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(clientModel->getHeaderTipTime()));
        setNumBlocks(clientModel->getNumBlocks(), clientModel->getLastBlockDate(), clientModel->getVerificationProgress(NULL), false);
        connect(clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(setNumBlocks(int,QDateTime,double,bool)));

        connect(clientModel, SIGNAL(additionalDataSyncProgressChanged(double)), this, SLOT(setAdditionalDataSyncProgress(double)));

        // Receive and report messages from client model
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        rpcConsole->setClientModel(clientModel);
#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(clientModel->getOptionsModel());
        
        OptionsModel* optionsModel = clientModel->getOptionsModel();
        if(optionsModel)
        {
            // be aware of the tray icon disable state change reported by the OptionsModel object.
            connect(optionsModel,SIGNAL(hideTrayIconChanged(bool)),this,SLOT(setTrayIconVisible(bool)));
        
            // initialize the disable state of the tray icon with the current value in the model.
            setTrayIconVisible(optionsModel->getHideTrayIcon());
        }
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
        // Propagate cleared model to child objects
        rpcConsole->setClientModel(nullptr);
#ifdef ENABLE_WALLET
        walletFrame->setClientModel(nullptr);
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(nullptr);

#ifdef Q_OS_MAC
        if(dockIconMenu)
        {
            // Disable context menu on dock icon
            dockIconMenu->clear();
        }
#endif
    }
}

#ifdef ENABLE_WALLET
bool MassGridGUI::addWallet(const QString& name, WalletModel *walletModel)
{
    if(!walletFrame)
        return false;
    m_walletModel = walletModel;
    setWalletActionsEnabled(true);
    updateAddr(walletModel);
    return walletFrame->addWallet(name, walletModel);
}

bool MassGridGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

void MassGridGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void MassGridGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
        masternodeAction->setEnabled(enabled);
    }
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
}

void MassGridGUI::createTrayIcon(const NetworkStyle *networkStyle)
{
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("MassGrid client") + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getTrayAndWindowIcon());
    trayIcon->hide();
#ifndef Q_OS_MAC
    // createActions();
    createTrayIconMenu();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void MassGridGUI::createTrayIconMenu()
{

    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    // trayIconMenu->setMinimumSize();
    trayIcon->setContextMenu(trayIconMenu);
    createIconMenu(trayIconMenu);

#ifndef Q_OS_MAC
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    dockIconMenu = dockIconHandler->dockMenu();
    createIconMenu(dockIconMenu);
#endif

}

void MassGridGUI::createIconMenu(QMenu *pmenu)
{
    // Configuration of the tray icon (or dock icon) icon menu
    pmenu->addAction(toggleHideAction);
    pmenu->addSeparator();
    pmenu->addAction(sendCoinsMenuAction);
    pmenu->addAction(receiveCoinsMenuAction);
    pmenu->addSeparator();
    pmenu->addAction(signMessageAction);
    pmenu->addAction(verifyMessageAction);
    pmenu->addSeparator();
    pmenu->addAction(optionsAction);
    pmenu->addAction(openInfoAction);
    pmenu->addAction(openRPCConsoleAction);
    pmenu->addAction(openGraphAction);
    pmenu->addAction(openPeersAction);
    pmenu->addAction(openRepairAction);
    pmenu->addSeparator();
    pmenu->addAction(openConfEditorAction);
    pmenu->addAction(openMNConfEditorAction);
    pmenu->addAction(showBackupsAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    pmenu->addSeparator();
    pmenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void MassGridGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#endif

void MassGridGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(0, enableWallet);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.setWalletModel(m_walletModel);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);
    dlg.exec();
}

void MassGridGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(0, HelpMessageDialog::about);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);
    dlg.exec();
}

void MassGridGUI::showDebugWindow()
{
    rpcConsole->showNormal();
    rpcConsole->show();
    rpcConsole->move(this->x()+(this->width()-rpcConsole->width())/2,this->y()+(this->height()-rpcConsole->height())/2);
    rpcConsole->raise();
    rpcConsole->activateWindow();
}

void MassGridGUI::showInfo()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_INFO);
    showDebugWindow();
}

void MassGridGUI::showConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_CONSOLE);
    showDebugWindow();
}

void MassGridGUI::showGraph()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_GRAPH);
    showDebugWindow();
}

void MassGridGUI::showPeers()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_PEERS);
    showDebugWindow();
}

void MassGridGUI::showRepair()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_REPAIR);
    showDebugWindow();
}

void MassGridGUI::showConfEditor()
{
    GUIUtil::openConfigfile();
}

void MassGridGUI::showMNConfEditor()
{
    GUIUtil::openMNConfigfile();
}

void MassGridGUI::showBackups()
{
    GUIUtil::showBackups();
}

void MassGridGUI::showHelpMessageClicked()
{
    helpMessageDialog->move(this->x()+(this->width()-helpMessageDialog->width())/2,this->y()+(this->height()-helpMessageDialog->height())/2);
    helpMessageDialog->show();
}

void MassGridGUI::showPrivateSendHelpClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(0, HelpMessageDialog::pshelp);
    dlg.exec();
}

#ifdef ENABLE_WALLET
void MassGridGUI::openClicked()
{
    OpenURIDialog dlg(0);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void MassGridGUI::importPrivkey()
{
    CAmount total = m_mainTitle->getTotal();
    LogPrintf("MassGridGUI::importPrivkey total:%d\n",total);
    if(total>0){
        CMessageBox::information(this, tr("Import private key"),tr("Import the private key will override the original one, please make sure the wallet balance is 0 and then do this.")); //导入私钥将覆盖原来的私钥，请确保钱包余额为0再进行此操作
        return ;
    }
    else{
        CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Import private key"),
            tr("Importing the private key deletes the existing receive address. This process is irreversible. Please confirm whether to do that?"),
            CMessageBox::Ok_Cancel, CMessageBox::Cancel);
        if(btnRetVal == CMessageBox::Cancel)
            return;
    }


    AddressTableModel* addressModel = m_walletModel->getAddressTableModel();
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(addressModel);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Receive);

    int rowCount = proxyModel->rowCount();
    for(int i =0;i<rowCount;i++)
        proxyModel->removeRow(0);

    PrivKeyMgr dlg(true,this,0);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);
    dlg.exec();
}

void MassGridGUI::dumpPrivkey()
{
    PrivKeyMgr dlg(false,this,0);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);
    dlg.exec();
}

void MassGridGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void MassGridGUI::gotoHistoryPage()
{
    m_mainTitle->setTransactionButtonStyle();
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void MassGridGUI::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction->setChecked(true);
        if (walletFrame) walletFrame->gotoMasternodePage();
    }
}

void MassGridGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void MassGridGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void MassGridGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void MassGridGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif // ENABLE_WALLET

void MassGridGUI::updateNetworkState()
{
    int count = clientModel->getNumConnections();
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }

    if (clientModel->getNetworkActive()) {
        labelConnectionsIcon->setToolTip(tr("%n active connection(s) to MassGrid network", "", count));
    } else {
        labelConnectionsIcon->setToolTip(tr("Network activity disabled"));
        icon = ":/icons/network_disabled";
    }

    labelConnectionsIcon->setPixmap(platformStyle->SingleColorIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
}

void MassGridGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void MassGridGUI::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void MassGridGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void MassGridGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{
    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }
    if (!clientModel)
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    // statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network..."));
            updateHeadersSyncProgressLabel();
            break;
        case BLOCK_SOURCE_DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk..."));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk..."));
            }
            break;
        case BLOCK_SOURCE_REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting to peers..."));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

#ifdef ENABLE_WALLET
    if (walletFrame)
    {
        if(secs < 25*60) // 90*60 in massgrid
        {
            modalOverlay->showHide(true, true);
            // TODO instead of hiding it forever, we should add meaningful information about MN sync to the overlay
            modalOverlay->hideForever();
        }
        else
        {
            modalOverlay->showHide();
        }
    }
#endif // ENABLE_WALLET

    if(!masternodeSync.IsBlockchainSynced())
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
                ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(true);
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void MassGridGUI::setAdditionalDataSyncProgress(double nSyncProgress)
{
    if(!clientModel)
        return;

    // No additional data sync should be happening while blockchain is not synced, nothing to update
    if(!masternodeSync.IsBlockchainSynced())
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    // statusBar()->clearMessage();

    QString tooltip;

    QString strSyncStatus;
    tooltip = tr("Up to date") + QString(".<br>") + tooltip;

    if(masternodeSync.IsSynced()) {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
        
        labelBlocksIcon->setPixmap(QIcon(":/icons/transaction_confirmed").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    } else {

        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
            ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
            .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

        progressBar->setFormat(tr("Synchronizing additional data: %p%"));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nSyncProgress * 1000000000.0 + 0.5);
    }

    strSyncStatus = QString(masternodeSync.GetSyncStatus().c_str());
    progressBarLabel->setText(strSyncStatus);
    tooltip = strSyncStatus + QString("<br>") + tooltip;

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void MassGridGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("MassGrid"); // default title
    // Default to information icon
    int nNotifyIcon = Notificator::Information;

    QString msgType;

    // Prefer supplied title over style based title
    if (!title.isEmpty()) {
        msgType = title;
    }
    else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            break;
        default:
            break;
        }
    }
    // Append title to "MassGrid - "
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        // QMessageBox::StandardButton buttons;
        // if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
        //     buttons = QMessageBox::Ok;

        // showNormalIfMinimized();
        // QMessageBox mBox(strTitle, message, buttons, this);
        // int r = mBox.exec();
        // if (ret != NULL)
        //     *ret = r == QMessageBox::Ok;


        showNormalIfMinimized();

        // QMessageBox mBox(strTitle, message, buttons, this);
        // int r = mBox.exec();

        CMessageBox::StandardButton btnRetVal = CMessageBox::information(this, strTitle,message);

        if (ret != NULL)
            *ret = btnRetVal == CMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

    // CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Create Failed"),
    //     msg,CMessageBox::Ok_Cancel, CMessageBox::Cancel);

    // if(btnRetVal == CMessageBox::Cancel){
    //     close();
    //     return ;
    // }

void MassGridGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void MassGridGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            QApplication::quit();
        }
    }
#endif
    QMainWindow::closeEvent(event);
}

void MassGridGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openInfoAction->setEnabled(true);
    openRPCConsoleAction->setEnabled(true);
    openGraphAction->setEnabled(true);
    openPeersAction->setEnabled(true);
    openRepairAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void MassGridGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(MassGridUnits::formatWithUnit(unit, amount, true)) +
                  tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             msg, CClientUIInterface::MSG_INFORMATION);
}
#endif // ENABLE_WALLET

void MassGridGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MassGridGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        Q_FOREACH(const QUrl &uri, event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool MassGridGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

#ifdef ENABLE_WALLET
bool MassGridGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void MassGridGUI::setHDStatus(int hdEnabled)
{
    labelWalletHDStatusIcon->setPixmap(platformStyle->SingleColorIcon(hdEnabled ? ":/icons/res/icons/hd_enabled" : ":/icons/res/icons/hd_disabled").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelWalletHDStatusIcon->setToolTip(hdEnabled ? tr("HD key generation is <b>enabled</b>") : tr("HD key generation is <b>disabled</b>"));

    // eventually disable the QLabel to set its opacity to 50%
    labelWalletHDStatusIcon->setEnabled(hdEnabled);
}

void MassGridGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/res/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::UnlockedForMixingOnly:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/res/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for mixing only"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/res/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}
#endif // ENABLE_WALLET

void MassGridGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void MassGridGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void MassGridGUI::detectShutdown()
{
    if (ShutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }
}

void MassGridGUI::showProgress(const QString &title, int nProgress)
{
    LogPrintf("MassGridGUI::showProgress nprogress:%d\n",nProgress);

    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(tr("Load progress"), "", 0, 100,this);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        // progressDialog->setStyleSheet("QProgressBar {\nborder: none;\n text-align: center;\ncolor: white;\nbackground-color: rgb(172, 99, 43);\nbackground-repeat: repeat-x;\ntext-align: center;}\nQProgressBar::chunk {\nborder: none;\nbackground-color: rgb(239, 169, 4);\nbackground-repeat: repeat-x;\n}");
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
        // progressDialog->move(10,10);
        progressDialog->move(this->x()+(this->width()-progressDialog->width())/2,this->y()+(this->height()-progressDialog->height())/2);
    }
    else if (nProgress == 100)
    {
        // if (progressDialog)
        // {
        //     progressDialog->close();
        //     progressDialog->deleteLater();
        // }
        closeProgress();
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);

    progressDialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    progressDialog->show();
}

void MassGridGUI::closeProgress()
{
    if (progressDialog)
    {
        progressDialog->close();
        progressDialog->deleteLater();
    }
}

void MassGridGUI::updateClient(QString version,bool stopMinerFlag)
{
    openWebUrl(version,stopMinerFlag);
}

void MassGridGUI::checkoutUpdateClient()
{
    QString version;
    bool stopMinerFlag = false;
    bool needUpdate = false;
    CUpdateThread::ChecketUpdate(needUpdate,version,stopMinerFlag);

    if(needUpdate)
        updateClient(version,stopMinerFlag);
    else
        CMessageBox::information(this, tr("Soft Update"),tr("Your version is up to date."));

// Your version is up to date.
}

void MassGridGUI::setTrayIconVisible(bool fHideTrayIcon)
{
    if (trayIcon)
    {
        trayIcon->setVisible(!fHideTrayIcon);
    }
}

void MassGridGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(MassGridGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    return ret;
}

void MassGridGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    uiInterface.ThreadSafeQuestion.connect(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
}

void MassGridGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    uiInterface.ThreadSafeQuestion.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
}

// QString::fromStdString(FormatFullVersion());
// v1.1.3.0-41ff824 (64 λ)
void MassGridGUI::openWebUrl(const QString& version,bool stopMinerFlag)
{
    //get network checkout 
    //TODO:If need to stop rpc
    QString clientversion = QString::fromStdString(FormatFullVersion()).split("-").first();
    LogPrintf("MassGridGUI::openWebUrl clientversion:%s\n",clientversion.toStdString().c_str());
    // QString leftversion = 
     
    QString curVersion = clientversion.mid(1).replace(".","");
    QString netVersion = version.mid(1).replace(".","");
    LogPrintf("MassGridGUI::openWebUrl curVersion:%s\n",curVersion.toStdString().c_str());
    LogPrintf("MassGridGUI::openWebUrl netVersion:%s\n",netVersion.toStdString().c_str());
    if(curVersion >= netVersion)
        return ;

    QString stopMinerStr = "";
    if(stopMinerFlag){
        StopMiner();
        // StopRPCThreads();
        stopMinerStr = "\n"+tr("The mining process has been shut down.");
    }

    CMessageBox::information(this, tr("Soft Update"),
                tr("Checkout an Update,version is %1. %2").arg(version).arg(stopMinerStr) + "<br><br>" + 

                tr("We will open the downloads url,or you can open this url to download the new Application.") + "<br><br>" + 
                "https://www.massgrid.com/downloads.html?language=en");
                        
    QDesktopServices::openUrl(QUrl("https://www.massgrid.com/downloads.html?language=en"));
    if(stopMinerFlag)
        QApplication::quit();
}

bool MassGridGUI::copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist)
{
    toDir.replace("\\","/");
    QString filename = sourceDir.split("/").last();
    filename = "wallet.dat";
    toDir.append(QString("/%1").arg(filename));
    if (sourceDir == toDir){
        return true;
    }
    if (!QFile::exists(sourceDir)){
        return false;
    }
    QDir createfile;
    bool exist = createfile.exists(toDir);
    if (exist){
        if(coverFileIfExist){
            createfile.remove(toDir);
        }
    }//end if

    if(!QFile::copy(sourceDir, toDir))
    {
        return false;
    }
    return true;
}

void MassGridGUI::showTipMessages()
{
    // if(GUIUtil::isNeedToShowTip()){
    //         CMessageBox::information(this, tr("新增特性"),
    //             tr("1.更新UI界面;") + "<br><br>" + 
    //             tr("2.修改了数据目录的默认安装路径（window系统下为C:\\user\\Documents\\MassGridDataDir;Linux系统下为/home/user/MassGridDataDir）,并在前端中显示该路径,用户可以通过\"设置->选项->显示->数据目录路径\"查到数据目录路径;") + "<br><br>" + 
    //             tr("3.新增新版本检测更新功能，用户需要安装最新版的钱包，以确保能正常挖矿;") + "<br><br>" + 
    //             tr("4.新增导入钱包文件功能，该操作会覆盖本地的钱包文件，用户需要谨慎操作;") + "<br><br>" + 
    //             tr("5.新增导入，导出私钥功能;") + "<br><br>" + 
    //             tr("6.新版钱包将只有一条收款地址,并且第一次使用新钱包的时候需要设置默认收款地址,可在 \"设置->选项->钱包->默认接收地址\"中执行此操作(原来的钱包由于有找零机制,界面上虽然显示一条收款地址,实际上钱包文件中会有多条地址,这样导致的情况是导出私钥时金额不一定是钱包总额,新版钱包去除了找零机制，限制只有一条收款地址，以确保钱包金额在一条私钥上);"));

    //     // CMessageBox::information(this, tr("The new features && Attention"),msg); //导入私钥将覆盖原来的私钥，请确保钱包余额为0再进行此操作
    // }
    startUpdateThread();
}



void MassGridGUI::toggleNetworkActive()
{
    if (clientModel) {
        clientModel->setNetworkActive(!clientModel->getNetworkActive());
    }
}

/** Get restart command-line parameters and request restart */
void MassGridGUI::handleRestart(QStringList args)
{
    if (!ShutdownRequested())
        Q_EMIT requestedRestart(args);
}

void MassGridGUI::mousePressEvent(QMouseEvent *e)
{
    m_last = e->globalPos();
}

void MassGridGUI::mouseMoveEvent(QMouseEvent *e)
{
    if(!(m_mainTitle && m_mainTitle->pressFlag())){
        return ;
    }

    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    m_winPos = QPoint(this->x()+dx, this->y()+dy);
    this->move(m_winPos);
}

void MassGridGUI::mouseReleaseEvent(QMouseEvent *e)
{
    if(!(m_mainTitle && m_mainTitle->pressFlag())){
        return ;
    }
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_winPos = QPoint(this->x()+dx, this->y()+dy);
    this->move(m_winPos);
}

void MassGridGUI::resizeEvent(QResizeEvent*)
{
    m_winSize = this->size();
}


void MassGridGUI::startUpdateThread()
{
    if(m_updateClientThread == 0){
        m_updateClientThread = new CUpdateThread(this);
        connect(m_updateClientThread,SIGNAL(updateClient(QString,bool)),this,SLOT(updateClient(QString,bool)));
    }
    LogPrintStr("start to scan update client.\n");
    m_updateClientThread->start();
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl(const PlatformStyle *platformStyle) :
    optionsModel(0),
    menu(0)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<MassGridUnits::Unit> units = MassGridUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(font());
    Q_FOREACH (const MassGridUnits::Unit unit, units)
    {
        max_width = qMax(max_width, fm.width(MassGridUnits::name(unit)));
    }
    setMinimumSize(max_width, 0);
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    // setStyleSheet(QString("QLabel { color : %1 }").arg(platformStyle->SingleColor().name()));
    setStyleSheet(QString("QLabel { color : rgb(255,255,255); }"));;
}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *event)
{
    onDisplayUnitsClicked(event->pos());
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu(this);
    Q_FOREACH(MassGridUnits::Unit u, MassGridUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(MassGridUnits::name(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }
    connect(menu,SIGNAL(triggered(QAction*)),this,SLOT(onMenuSelection(QAction*)));
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *optionsModel)
{
    if (optionsModel)
    {
        this->optionsModel = optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(optionsModel,SIGNAL(displayUnitChanged(int)),this,SLOT(updateDisplayUnit(int)));

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    setText(MassGridUnits::name(newUnits));
}

/** Shows context menu with Display Unit options by the mouse coordinates */
void UnitDisplayStatusBarControl::onDisplayUnitsClicked(const QPoint& point)
{
    QPoint globalPos = mapToGlobal(point);
    menu->exec(globalPos);
}

/** Tells underlying optionsModel to update its current display unit. */
void UnitDisplayStatusBarControl::onMenuSelection(QAction* action)
{
    if (action)
    {
        optionsModel->setDisplayUnit(action->data());
    }
}
