// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "massgridgui.h"

#include "massgridunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "networkstyle.h"
#include "notificator.h"
#include "openuridialog.h"
#include "optionsdialog.h"
#include "optionsmodel.h"
#include "rpcconsole.h"
#include "utilitydialog.h"
#include "miner.h"
#include "cupdatethread.h"
#include "cprogressdialog.h"
#ifdef ENABLE_WALLET
#include "walletframe.h"
#include "walletmodel.h"
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include "init.h"
#include "ui_interface.h"
#include "util.h"
#include "cmessagebox.h"
#include "privkeymgr.h"
#include "rpcserver.h"
#include "addresstablemodel.h"

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QIcon>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>

#include <QMimeData>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
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

#if QT_VERSION < 0x050000
#include <QTextDocument>
#include <QUrl>
#else
#include <QUrlQuery>
#endif

const QString MassGridGUI::DEFAULT_WALLET = "~Default";
QPoint m_winPos;
QSize m_winSize;

MassGridGUI::MassGridGUI(const NetworkStyle *networkStyle, QWidget *parent) :
    QWidget(parent),
    clientModel(0),
    walletFrame(0),
    unitDisplayControl(0),
    labelEncryptionIcon(0),
    labelConnectionsIcon(0),
    labelBlocksIcon(0),
    progressBarLabel(0),
    progressBar(0),
    progressDialog(0),
    // appMenuBar(0),
    overviewAction(0),
    historyAction(0),
    quitAction(0),
    sendCoinsAction(0),
    usedSendingAddressesAction(0),
    usedReceivingAddressesAction(0),
    signMessageAction(0),
    verifyMessageAction(0),
    aboutAction(0),
    receiveCoinsAction(0),
    optionsAction(0),
    toggleHideAction(0),
    encryptWalletAction(0),
    backupWalletAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    openRPCConsoleAction(0),
    openAction(0),
    showHelpMessageAction(0),
    importPrivKeyAction(0),
    dumpPrivKeyAction(0),
    trayIcon(0),
    trayIconMenu(0),
    notificator(0),
    rpcConsole(0),
    prevBlocks(0),
    spinnerFrame(0),
    m_mainTitle(0),
    m_updateClientThread(0)

{
    GUIUtil::restoreWindowGeometry("nWindow", QSize(850,650),this);

    QString windowTitle = tr("MassGrid Core") + " - ";
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
    windowTitle += " " + networkStyle->getTitleAddText();
#ifndef Q_OS_MAC
    QApplication::setWindowIcon(networkStyle->getAppIcon());
    setWindowIcon(networkStyle->getAppIcon());
#else
    MacDockIconHandler::instance()->setIcon(networkStyle->getAppIcon());
#endif
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC) && QT_VERSION < 0x050000
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    rpcConsole = new RPCConsole(enableWallet ? 0 : 0);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        // walletFrame = new WalletFrame(this);
        // setCentralWidget(walletFrame);

        createMainWin(networkStyle);

    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        // setCentralWidget(rpcConsole);


        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget((rpcConsole));
        // layout->addWidget(walletFrame);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        // win->setLayout(layout);
        // this->setCentralWidget(win);
        this->setLayout(layout);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create system tray icon and notification
    createTrayIcon(networkStyle);


    // Create application menu bar
    if(m_mainTitle)
        createMenuBar();

    // Create the toolbars 
    // createToolBars(); 


    // Create status bar
    // statusBar();

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    unitDisplayControl = new UnitDisplayStatusBarControl();
    labelEncryptionIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelEncryptionIcon);
    }
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html

    // QString curStyle = QApplication::style()->metaObject()->className();
    // if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    // {
    //     progressBar->setStyleSheet("QProgressBar {\nborder: none;\ntext-align: center;\ncolor: white;\nbackground-color: rgb(172, 99, 43);\nbackground-repeat: repeat-x;\ntext-align: center;}\nQProgressBar::chunk {\nborder: none;\nbackground-color: rgb(239, 169, 4);\nbackground-repeat: repeat-x;\n}");
    //     // progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    // }

    progressBar->setStyleSheet("QProgressBar {\nborder: none;\n min-height:30px; min-width:500px; max-height:30px; text-align: center;\ncolor: white;\nbackground-color: rgb(172, 99, 43);\nbackground-repeat: repeat-x;\ntext-align: center;}\nQProgressBar::chunk {\nborder: none;\nbackground-color: rgb(239, 169, 4);\nbackground-repeat: repeat-x;\n}");
    statusFrame->setObjectName("statusFrame");
    statusFrame->setStyleSheet("QFrame#statusFrame {  \n    background-color: rgb(247, 242, 238);\n\n}  ");

    QHBoxLayout *statusFrameLayout = new QHBoxLayout(statusFrame);
    statusFrame->setLayout(statusFrameLayout);

    statusFrameLayout->setContentsMargins(9,0,0,0);
    statusFrameLayout->setSpacing(0);

    QSpacerItem* spacerItem = new QSpacerItem(850,20);

    statusFrameLayout->addWidget(progressBarLabel);
    statusFrameLayout->addWidget(progressBar);
    statusFrameLayout->addSpacerItem(spacerItem);
    statusFrameLayout->addWidget(frameBlocks);

    // statusBar()->setStyleSheet("QStatusBar {  \n    background-color: rgb(247, 242, 238);\n\n}  ");
    // statusBar()->addWidget(progressBarLabel);
    // statusBar()->addWidget(progressBar);
    // statusBar()->addPermanentWidget(frameBlocks);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    setWindowFlags(Qt::FramelessWindowHint);
    resize(850,650);
    m_winPos = this->pos();
    m_winSize = this->size();

    /*start time to open update thread*/
    QTimer::singleShot(10000, this, SLOT(startUpdateThread()));
}

MassGridGUI::~MassGridGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();

    if(m_updateClientThread != 0){
        QEventLoop loop;
        connect(m_updateClientThread,SIGNAL(threadStoped()),&loop,SLOT(quit()));
        m_updateClientThread->stopThread();
        loop.exec();
        m_updateClientThread->quit();
        m_updateClientThread->wait();
        delete m_updateClientThread;
    }

// #ifdef Q_OS_MAC
//     delete appMenuBar;
//     MacDockIconHandler::cleanup();
// #endif
}

void MassGridGUI::createMainWin(const NetworkStyle *networkStyle)
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

    // connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    // connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    // connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    // connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    // connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    // connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    // connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    // connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));

#endif // ENABLE_WALLET

    walletFrame = new WalletFrame(this);

    statusFrame = new QFrame(this);
    statusFrame->setStyleSheet("background-color:rgb(255,255,0);");
    // statusFrame->setMaximumSize(100000,40);
    statusFrame->setMinimumSize(800,40);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget((m_mainTitle));
    layout->addWidget(walletFrame);
    layout->addWidget(statusFrame);

    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    // win->setLayout(layout);
    // this->setCentralWidget(win);
    // this->setLayout(layout);
    mainFrame->setLayout(layout);
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
        if(backgroudlayout)
            backgroudlayout->setContentsMargins(40,40,40,40);
        this->showNormal();

    }
    else{
        if(backgroudlayout)
            backgroudlayout->setContentsMargins(0,0,0,0);
        this->showMaximized();
    }
}  


void MassGridGUI::createBackgroundWin()
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    QGridLayout* layout = new QGridLayout(this);
    backgroudlayout = new QGridLayout(this);
    this->setLayout(layout);

    layout->setContentsMargins(0,0,0,0);
    QWidget * centerWin = new QWidget(this);
    centerWin->setObjectName("centerWin");

    layout->addWidget(centerWin);

    centerWin->setStyleSheet("QWidget#centerWin{\n  border-image: url(:/pic/res/pic/dlgBackgroud.png);\n}");

    centerWin->setLayout(backgroudlayout);

    mainFrame = new QFrame(this);
    mainFrame->setObjectName("mainFrame");

    mainFrame->setStyleSheet("QFrame#mainFrame{\n   background-color: rgb(255, 255, 255);\n}");
    mainFrame->setMinimumSize(850,650);

    backgroudlayout->addWidget(mainFrame);
    backgroudlayout->setContentsMargins(40,40,40,40);
    centerWin->setLayout(backgroudlayout);
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

void MassGridGUI::createActions(const NetworkStyle *networkStyle)
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a MassGrid address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and massgrid: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    m_mainTitle->setButtonText(tr("&Overview"),tr("&Send"),tr("&Transactions"));

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
#endif // ENABLE_WALLET

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    // networkStyle->getAppIcon()
    aboutAction = new QAction( QIcon(":/pic/res/pic/menuicon/about.png"), tr("&About MassGrid Core"), this);
    aboutAction->setStatusTip(tr("Show information about MassGrid Core"));
    aboutAction->setMenuRole(QAction::AboutRole);
#if QT_VERSION < 0x050000
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
#else
    aboutQtAction = new QAction(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
#endif
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/pic/res/pic/menuicon/options.png"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for MassGrid"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(networkStyle->getAppIcon(), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(QIcon(":/pic/res/pic/menuicon/lock_closed.png"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/pic/res/pic/menuicon/filesave.png"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));

    inputWalletAction = new QAction(QIcon(":/pic/res/pic/menuicon/importwallet.png"), tr("&Import Wallet..."), this);
    inputWalletAction->setStatusTip(tr("Import wallet file"));

    changePassphraseAction = new QAction(QIcon(":/pic/res/pic/menuicon/key.png"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    signMessageAction = new QAction(QIcon(":/pic/res/pic/menuicon/edit.png"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your MassGrid addresses to prove you own them"));
    verifyMessageAction = new QAction(QIcon(":/pic/res/pic/menuicon/verifyMessage.png"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified MassGrid addresses"));

    openRPCConsoleAction = new QAction(QIcon(":/pic/res/pic/menuicon/debugwindow.png"), tr("&Debug window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));

    usedSendingAddressesAction = new QAction(QIcon(":/pic/res/pic/menuicon/address-book.png"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));
    //QApplication::style()->standardIcon(QStyle::SP_FileIcon)
    openAction = new QAction(QIcon(":/pic/res/pic/menuicon/openFile.png"), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a massgrid: URI or payment request"));
    //QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation)

    showHelpMessageAction = new QAction(QIcon(":/pic/res/pic/menuicon/command.png") , tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the MassGrid Core help message to get a list with possible MassGrid command-line options"));

    importPrivKeyAction = new QAction(QIcon(":/pic/res/pic/menuicon/importprivkey.png"), tr("&Import privkey"), this);
    importPrivKeyAction->setStatusTip(tr("import the private key."));

    dumpPrivKeyAction = new QAction(QIcon(":/pic/res/pic/menuicon/dumpprivkey.png"), tr("&Dump privkey"), this);
    dumpPrivKeyAction->setStatusTip(tr("dump out the private key."));

    softUpdateAction = new QAction(QIcon(":/pic/res/pic/menuicon/softupdate.png"), tr("&Soft Update"), this);
    softUpdateAction->setStatusTip(tr("Soft Update"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
    connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showDebugWindow()));
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(inputWalletAction, SIGNAL(triggered()), this, SLOT(inputWalletFile()));
        connect(softUpdateAction, SIGNAL(triggered()), this, SLOT(checkoutUpdateClient()));
        
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));

        connect(importPrivKeyAction, SIGNAL(triggered()), this, SLOT(importPrivkey()));
        connect(dumpPrivKeyAction, SIGNAL(triggered()), this, SLOT(dumpPrivkey()));

    }
#endif // ENABLE_WALLET
}


void MassGridGUI::createMenuBar()
{
// #ifdef Q_OS_MAC
//     // Create a decoupled menu bar on Mac which stays even if the window is closed
//     appMenuBar = new QMenuBar();
// #else
//     // Get the main window's menu bar on other platforms
//     appMenuBar = menuBar();
// #endif

    //     QMenu* getfileMenu();
    // QMenu* settingsMenu();
    // QMenu* helpMenu();

    // Configure the menus
    // QMenu *file = appMenuBar->addMenu(tr("&File"));
    QMenu* file = m_mainTitle->fileMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(inputWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addAction(importPrivKeyAction);
        file->addAction(dumpPrivKeyAction);
        file->addAction(softUpdateAction);
        // file->addSeparator();
    }
    // file->addAction(quitAction);

    // QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    QMenu* settings = m_mainTitle->settingsMenu(tr("&Settings"));

    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    // QMenu *help = appMenuBar->addMenu(tr("&Help"));
    QMenu *help = m_mainTitle->helpMenu(tr("&Help"));
    if(walletFrame)
    {
        help->addAction(openRPCConsoleAction);
    }
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void MassGridGUI::createToolBars()
{
    if(walletFrame)
    {
        // QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
        // toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        // toolbar->addAction(overviewAction);
        // toolbar->addAction(sendCoinsAction);
        // toolbar->addAction(receiveCoinsAction);
        // toolbar->addAction(historyAction);
        overviewAction->setChecked(true);
    }
}

void MassGridGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        // createTrayIconMenu();

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

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
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
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
    receiveCoinsAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
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
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("MassGrid Core client") + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getAppIcon());
    trayIcon->show();

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions(networkStyle);
    createTrayIconMenu();
    trayIcon->show();
#endif


    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);


}

void MassGridGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-Mac OSes)
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);

    trayIconMenu->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background:rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n  margin-right: 2px;\n    margin-top: 2px;\n  margin-bottom: 2px;\n    height:30px;\n}\n \nQMenu::item:selected:enabled{\n    background-color: rgb(239,169,4); \n    color: white;            \n}\n \nQMenu::item:selected:!enabled{\n    background:transparent;\n}");

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QWidget *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
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

QPoint MassGridGUI::winPos()
{
    return m_winPos;
}

QSize MassGridGUI::winSize()
{
    return m_winSize;
}

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

    HelpMessageDialog dlg(0, true);

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

void MassGridGUI::showHelpMessageClicked()
{
    HelpMessageDialog *help = new HelpMessageDialog(0, false);
    help->setAttribute(Qt::WA_DeleteOnClose);
    help->move(this->x()+(this->width()-help->width())/2,this->y()+(this->height()-help->height())/2);

    help->show();
}

#ifdef ENABLE_WALLET
void MassGridGUI::openClicked()
{
    OpenURIDialog dlg(0);
    dlg.move(this->x()+(this->width()-dlg.width())/2,this->y()+(this->height()-dlg.height())/2);

    if(dlg.exec())
    {
        emit receivedURI(dlg.getURI());
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

void MassGridGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to MassGrid network", "", count));
}

void MassGridGUI::setNumBlocks(int count)
{
    if(!clientModel)
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    // statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            progressBarLabel->setText(tr("Synchronizing with network..."));
            break;
        case BLOCK_SOURCE_DISK:
            progressBarLabel->setText(tr("Importing blocks from disk..."));
            break;
        case BLOCK_SOURCE_REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
            progressBarLabel->setText(tr("No block source available..."));
            break;
    }

    QString tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = lastBlockDate.secsTo(currentDate);

    tooltip = tr("Processed %n blocks of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/pic/res/pic/selectPic.png").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE)); //:/icons/synced

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }
    else
    {
        // Represent time from last generated block in human readable text
        QString timeBehindText;
        const int HOUR_IN_SECONDS = 60*60;
        const int DAY_IN_SECONDS = 24*60*60;
        const int WEEK_IN_SECONDS = 7*24*60*60;
        const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
        if(secs < 2*DAY_IN_SECONDS)
        {
            timeBehindText = tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
        }
        else if(secs < 2*WEEK_IN_SECONDS)
        {
            timeBehindText = tr("%n day(s)","",secs/DAY_IN_SECONDS);
        }
        else if(secs < YEAR_IN_SECONDS)
        {
            timeBehindText = tr("%n week(s)","",secs/WEEK_IN_SECONDS);
        }
        else
        {
            int years = secs / YEAR_IN_SECONDS;
            int remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
        }

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(clientModel->getVerificationProgress() * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(QIcon(QString(
                ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(true);
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

void MassGridGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("MassGrid"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
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
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons, this);
        int r = mBox.exec();
        if (ret != NULL)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void MassGridGUI::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
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
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            QApplication::quit();
        }
    }
#endif
    QWidget::closeEvent(event);
}

#ifdef ENABLE_WALLET
void MassGridGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address)
{
    // On new transaction, make an info balloon
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             tr("Date: %1\n"
                "Amount: %2\n"
                "Type: %3\n"
                "Address: %4\n")
                  .arg(date)
                  .arg(MassGridUnits::formatWithUnit(unit, amount, true))
                  .arg(type)
                  .arg(address), CClientUIInterface::MSG_INFORMATION);
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
        foreach(const QUrl &uri, event->mimeData()->urls())
        {
            emit receivedURI(uri.toString());
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
    return QWidget::eventFilter(object, event);
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

void MassGridGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
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
        progressDialog->setStyleSheet("QProgressBar {\nborder: none;\n text-align: center;\ncolor: white;\nbackground-color: rgb(172, 99, 43);\nbackground-repeat: repeat-x;\ntext-align: center;}\nQProgressBar::chunk {\nborder: none;\nbackground-color: rgb(239, 169, 4);\nbackground-repeat: repeat-x;\n}");
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
}

void MassGridGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}

void MassGridGUI::inputWalletFile()
{
    CMessageBox::StandardButton btnRetVal = CMessageBox::question(this, tr("Import Wallet"),
            tr("Import wallet file will cover the old one,please back up your old wallet."),
            CMessageBox::Ok_Cancel, CMessageBox::Cancel);

    if(btnRetVal == CMessageBox::Cancel)
        return;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                      "/home",
                                                      tr("Wallat (*.dat)"));

    if(fileName.isEmpty())
        return ;

    QString curdir = GetDataDir().string().c_str();
    // curdir+="/wallet.dat";

    // LogPrintf("MassGridGUI::inputWalletFile curdir:%s\n",curdir.toStdString().c_str());

    // copyFileToPath(fileName,curdir,true);
    if(copyFileToPath(fileName,curdir,true)){
        // qDebug() << "copy file sucess!";
        CMessageBox::warning(this, tr("Import Wallet"),
             "<qt>" +
             tr("MassGrid will close now to update the Wallet,Please restart your wallet later.")
             +"</qt>");
        QApplication::quit();
    }
    else{
        // qDebug() << "copy file fail!";
        CMessageBox::warning(this, tr("Import Error"),
             "<qt>" +
             tr("Import wallet error,please checkout the wallet file is exists.")
             +"</qt>");
    }
}

void MassGridGUI::openWebUrl(const QString& version,bool stopMinerFlag)
{
    //get network checkout 
    //TODO:If need to stop rpc
    if(stopMinerFlag){
        StopMiner();
        StopRPCThreads();
    }

    CMessageBox::information(this, tr("Soft Update"),
                tr("Checkout an Update,version is %1.").arg(version) + "<br><br>" + 
                tr("We will open the downloads url,or you can open this url to download the new Application.") + "<br><br>" + 
                "https://www.massgrid.com/downloads.html?language=en");
                        
    QDesktopServices::openUrl(QUrl("https://www.massgrid.com/downloads.html?language=en"));
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

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl() :
    optionsModel(0),
    menu(0)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *event)
{
    onDisplayUnitsClicked(event->pos());
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu();
    foreach(MassGridUnits::Unit u, MassGridUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(MassGridUnits::name(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }

    menu->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background:rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n  margin-right: 2px;\n    margin-top: 2px;\n  margin-bottom: 2px;\n    height:30px;\n}\n \nQMenu::item:selected:enabled{\n    background-color: rgb(239,169,4); \n    color: white;            \n}\n \nQMenu::item:selected:!enabled{\n    background:transparent;\n}");


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
    setPixmap(QIcon(":/icons/unit_" + MassGridUnits::id(newUnits)).pixmap(31,STATUSBAR_ICONSIZE));
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
