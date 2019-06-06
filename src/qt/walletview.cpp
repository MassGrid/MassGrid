// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"

#include "addressbookpage.h"
#include "askpassphrasedialog.h"
#include "massgridgui.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "masternodeconfig.h"
#include "optionsmodel.h"
#include "overviewpage.h"
#include "platformstyle.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "walletmodel.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "masternodelist.h"

#include "ui_interface.h"
#include "guiutil.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QStyleFactory>

MasternodeList *g_masternodeListPage;
SendCoinsDialog *g_sendCoinsPage;

WalletView::WalletView(const PlatformStyle *platformStyle, QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    walletModel(0),
    platformStyle(platformStyle)
{
    // Create tabs
    overviewPage = new OverviewPage(platformStyle);

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    transactionView = new TransactionView(platformStyle, this);
    vbox->addWidget(transactionView);
    QToolButton *exportButton = new QToolButton(this);
    // exportButton->setText(tr("&Export"));
    exportButton->setText(tr("Export"));
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));

    exportButton->setStyleSheet("border:hidde;\ncolor: rgb(172, 99, 43);");
    exportButton->setIcon(QIcon(":/res/pic/outputData.png"));
    exportButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);  

    QLineEdit *addressEdit = new QLineEdit(this);
    QComboBox *dateComboBox = new QComboBox(this);
    QComboBox *typeComboBox = new QComboBox(this);

    addressEdit->setMinimumSize(300,30*GUIUtil::GetDPIValue());

    addressEdit->setObjectName("addressEdit");
    addressEdit->setStyleSheet("QLineEdit::focus{\nborder:1px solid rgb(238,169,4);\nbackground-color: rgb(255, 255, 255,0);\n}\nQLineEdit::hover{\nborder:1px solid rgb(238,169,4);\nbackground-color: rgb(255, 255, 255,0);\n}\n\nQLineEdit\n{\nborder:1px solid rgb(165, 165, 165);\nbackground-color: rgb(255, 255, 255,0);\nborder-radius:4px;\n}");
    // addressEdit->setStyleSheet("QLineEdit\n{\nmin-width:300px;border:1px solid rgb(165, 165, 165);\nbackground-color: rgb(255, 255, 255,0);\n}\n\nQLineEdit::hover{\nborder:1px solid rgb(238,169,4);\nbackground-color: rgb(255, 255, 255,0);\n}");

#if QT_VERSION >= 0x040700
    addressEdit->setPlaceholderText(tr("Enter address or label to search"));
#endif

    transactionView->setSearchWidget(dateComboBox,typeComboBox,addressEdit);

#ifdef Q_OS_MAC
    typeComboBox->setStyle(QStyleFactory::create("Windows"));
    dateComboBox->setStyle(QStyleFactory::create("Windows"));

//     typeComboBox->setStyleSheet("QComboBox{\n    background-color:rgb(49, 61, 64); \n    color:white;\n    height:30px;\n    width:120px;\n    border:1px solid rgb(210,210,210);\n	border-radius:2px;\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n    width: 30px;\n    height:30px;\n    image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(49, 61, 64);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n  font:15pt;\n  background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
//     dateComboBox->setStyleSheet("QComboBox{\n    background-color:rgb(49, 61, 64); \n    color:white;\n    height:30px;\n    width:120px;\n    border:1px solid rgb(210,210,210);\n	border-radius:2px;\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n    width: 30px;\n    height:30px;\n    image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(49, 61, 64);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n  font:15pt;\n  background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
// #else
//     typeComboBox->setStyleSheet("QComboBox{\n    background-color:rgb(49, 61, 64); \n    color:white;\n    height:30px;\n    width:120px;\n    border:1px solid rgb(210,210,210);\n	border-radius:2px;\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n    width: 30px;\n    height:30px;\n    image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(49, 61, 64);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n  font:15pt;\n  background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
//     dateComboBox->setStyleSheet("QComboBox{\n    background-color:rgb(49, 61, 64); \n    color:white;\n    height:30px;\n    width:120px;\n    border:1px solid rgb(210,210,210);\n	border-radius:2px;\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n    width: 30px;\n    height:30px;\n    image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(49, 61, 64);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n  font:15pt;\n  background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
#endif

    connect(typeComboBox, SIGNAL(activated(int)), transactionView, SLOT(chooseType(int)));
    connect(dateComboBox, SIGNAL(activated(int)), transactionView, SLOT(chooseDate(int)));
    connect(addressEdit, SIGNAL(textChanged(QString)), transactionView, SLOT(changedPrefix(QString)));

    QLabel* label_2 = new QLabel(this);
    // label_2->setFixedWidth(60);

    label_2->setMinimumSize(160,10);
    label_2->setMaximumSize(160,10);

    // exportButton->setMinimumSize(80,30);
    // exportButton->setMaximumSize(80,30);
    

    exportButton->setIcon(QIcon(":/res/pic/outputData-yellow.png"));

#ifndef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    exportButton->setIcon(QIcon(":/res/pic/outputData-yellow.png"));
#endif
    hbox_buttons->addStretch();

    hbox_buttons->addWidget(typeComboBox);    
    hbox_buttons->addWidget(dateComboBox);
    hbox_buttons->addWidget(addressEdit);
    hbox_buttons->addWidget(label_2);

    hbox_buttons->addWidget(exportButton);
    vbox->addLayout(hbox_buttons);
    transactionsPage->setLayout(vbox);

    transactionsPage->setObjectName("transactionsPage");
    transactionsPage->setStyleSheet("QWidget#transactionsPage{\n background-color: rgb(255, 255, 255);\n}");
    receiveCoinsPage = new ReceiveCoinsDialog(platformStyle);
    g_sendCoinsPage = new SendCoinsDialog(platformStyle);

    usedSendingAddressesPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab, 0);
    usedReceivingAddressesPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, 0);

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(receiveCoinsPage);
    addWidget(g_sendCoinsPage);

    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        g_masternodeListPage = new MasternodeList(platformStyle);
        addWidget(g_masternodeListPage);
    }

    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));
    connect(overviewPage, SIGNAL(outOfSyncWarningClicked()), this, SLOT(requestedSyncWarningInfo()));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    // Update wallet with sum of selected transactions
    connect(transactionView, SIGNAL(trxAmount(QString)), this, SLOT(trxAmount(QString)));

    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));

    // Pass through messages from g_sendCoinsPage
    connect(g_sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

    // Pass through messages from transactionView
    connect(transactionView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
}

WalletView::~WalletView()
{
}

void WalletView::setMassGridGUI(MassGridGUI *gui)
{
    if (gui)
    {
        connect(overviewPage,SIGNAL(updateBalance(QString ,QString ,QString ,bool ,bool ,QString )),gui,SIGNAL(updateBalance(QString,QString,QString,bool,bool,QString)));
        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        connect(g_sendCoinsPage, SIGNAL(sendCoinSucess()), gui, SLOT(gotoHistoryPage()));
        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(encryptionStatusChanged(int)), gui, SLOT(setEncryptionStatus(int)));

        // Pass through transaction notifications
        connect(this, SIGNAL(incomingTransaction(QString,int,CAmount,QString,QString,QString)), gui, SLOT(incomingTransaction(QString,int,CAmount,QString,QString,QString)));

        // Connect HD enabled state signal
        connect(this, SIGNAL(hdEnabledStatusChanged(int)), gui, SLOT(setHDStatus(int)));
    }
}

void WalletView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    overviewPage->setClientModel(clientModel);
    g_sendCoinsPage->setClientModel(clientModel);
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        g_masternodeListPage->setClientModel(clientModel);
    }
}

void WalletView::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;

    // Put transaction list in tabs
    transactionView->setModel(walletModel);
    overviewPage->setWalletModel(walletModel);
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        g_masternodeListPage->setWalletModel(walletModel);
    }
    receiveCoinsPage->setModel(walletModel);
    g_sendCoinsPage->setModel(walletModel);
    usedReceivingAddressesPage->setModel(walletModel->getAddressTableModel());
    usedSendingAddressesPage->setModel(walletModel->getAddressTableModel());

    if (walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // update HD status
        Q_EMIT hdEnabledStatusChanged(walletModel->hdEnabled());

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock(bool)), this, SLOT(unlockWallet(bool)));

        // Show progress dialog
        connect(walletModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->inInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = ttm->data(index, TransactionTableModel::LabelRole).toString();

    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        setCurrentWidget(g_masternodeListPage);
    }
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(g_sendCoinsPage);

    if (!addr.isEmpty())
        g_sendCoinsPage->setAddress(addr);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, 0);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();

    signVerifyMessageDialog->move(pos.x()+(size.width()-signVerifyMessageDialog->width())/2,pos.y()+(size.height()-signVerifyMessageDialog->height())/2);
    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, 0);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();

    signVerifyMessageDialog->move(pos.x()+(size.width()-signVerifyMessageDialog->width())/2,pos.y()+(size.height()-signVerifyMessageDialog->height())/2);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return g_sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    Q_EMIT encryptionStatusChanged(walletModel->getEncryptionStatus());
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, 0);
    dlg.setModel(walletModel);
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();

    dlg.move(pos.x()+(size.width()-dlg.width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-dlg.height()*GUIUtil::GetDPIValue())/2);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        tr("Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!walletModel->backupWallet(filename)) {
        Q_EMIT message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        Q_EMIT message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, 0);
    dlg.setModel(walletModel);
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-dlg.height()*GUIUtil::GetDPIValue())/2);
    dlg.exec();
}

void WalletView::unlockWallet(bool fForMixingOnly)
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model

    if (walletModel->getEncryptionStatus() == WalletModel::Locked || walletModel->getEncryptionStatus() == WalletModel::UnlockedForMixingOnly)
    {
        AskPassphraseDialog dlg(fForMixingOnly ? AskPassphraseDialog::UnlockMixing : AskPassphraseDialog::Unlock, 0);
        dlg.setModel(walletModel);
        QPoint pos = MassGridGUI::winPos();
        QSize size = MassGridGUI::winSize();
        dlg.move(pos.x()+(size.width()-dlg.width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-dlg.height()*GUIUtil::GetDPIValue())/2);
        dlg.exec();
    }
}

void WalletView::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;

    usedSendingAddressesPage->show();
    usedSendingAddressesPage->raise();
    usedSendingAddressesPage->setModel(walletModel->getAddressTableModel());

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    usedSendingAddressesPage->move(pos.x()+(size.width()-usedSendingAddressesPage->width())/2,pos.y()+(size.height()-usedSendingAddressesPage->height())/2);
    usedSendingAddressesPage->activateWindow();
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;
		
	usedReceivingAddressesPage->setModel(walletModel->getAddressTableModel());
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    usedReceivingAddressesPage->move(pos.x()+(size.width()-usedReceivingAddressesPage->width())/2,pos.y()+(size.height()-usedReceivingAddressesPage->height())/2);	

    usedReceivingAddressesPage->show();
    usedReceivingAddressesPage->raise();
    usedReceivingAddressesPage->activateWindow();
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

void WalletView::requestedSyncWarningInfo()
{
    Q_EMIT outOfSyncWarningClicked();
}

/** Update wallet with the sum of the selected transactions */
void WalletView::trxAmount(QString amount)
{
    transactionSum->setText(amount);
}
