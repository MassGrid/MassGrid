// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"

#include "addressbookpage.h"
#include "askpassphrasedialog.h"
#include "massgridgui.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "overviewpage.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "walletmodel.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"

#include "ui_interface.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>

WalletView::WalletView(QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    walletModel(0)
{
    // Create tabs
    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    QToolButton *exportButton = new QToolButton(this);
    // exportButton->setText(tr("&Export"));
    exportButton->setText(tr("Export"));
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));

    exportButton->setStyleSheet("border:hidde;\ncolor: rgb(172, 99, 43);");
    exportButton->setIcon(QIcon(":/pic/res/pic/outputData.png"));
    exportButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);  

    QLineEdit *addressEdit = new QLineEdit(this);
    QComboBox *dateComboBox = new QComboBox(this);
    QComboBox *typeComboBox = new QComboBox(this);

    addressEdit->setMinimumSize(300,30);
    dateComboBox->setFixedWidth(120);
    dateComboBox->setMaximumSize(150,30);

    dateComboBox->setMinimumSize(120,32);
    dateComboBox->setMaximumSize(120,32);

    typeComboBox->setMinimumSize(120,32);
    typeComboBox->setMaximumSize(120,32);

    // dateComboBox->addItem(tr("All Date"), TransactionView::All);
    // dateComboBox->addItem(tr("Today"), TransactionView::Today);
    // dateComboBox->addItem(tr("This week"), TransactionView::ThisWeek);
    // dateComboBox->addItem(tr("This month"), TransactionView::ThisMonth);
    // dateComboBox->addItem(tr("Last month"), TransactionView::LastMonth);
    // dateComboBox->addItem(tr("This year"), TransactionView::ThisYear);
    // dateComboBox->addItem(tr("Range..."), TransactionView::Range);

    dateComboBox->setStyleSheet("QComboBox\n{\nwidth: 120px;  \nheight: 600px;\nborder:0px solid rgb(174,103,46);\nfont-size: 12pt;\nfont-family: 微软雅黑,宋体;\nbackground-repeat: no-repeat;\nbackground-position: center left;\nbackground-color: rgb(255, 255, 255,0);\ncolor: rgb(0, 0, 0);\nselection-color: black;\nselection-background-color: darkgray;\n}\n\nQComboBox::drop-down \n{\nwidth: 30px; \nheight:30px;\nimage: url(:/pic/res/pic/xjt.png);\n}\n\n");
    addressEdit->setStyleSheet("QLineEdit\n{\nmin-width:300px;border:1px solid rgb(165, 165, 165);\nbackground-color: rgb(255, 255, 255,0);\n}\n\nQLineEdit::hover{\nborder:1px solid rgb(174,103,46);\nbackground-color: rgb(255, 255, 255,0);\n}");
    typeComboBox->setStyleSheet("QComboBox\n{\nwidth: 120px;  \nheight: 600px;\nborder:0px solid rgb(174,103,46);\nfont-size: 12pt;\nfont-family: 微软雅黑,宋体;\nbackground-repeat: no-repeat;\nbackground-position: center left;\nbackground-color: rgb(255, 255, 255,0);\ncolor: rgb(0, 0, 0);\nselection-color: black;\nselection-background-color: darkgray;\n}\n\nQComboBox::drop-down \n{\nwidth: 30px; \nheight:30px;\nimage: url(:/pic/res/pic/xjt.png);\n}\n\n");

#if QT_VERSION >= 0x040700
    addressEdit->setPlaceholderText(tr("Enter address or label to search"));
#endif

// #ifdef Q_OS_MAC
//     typeComboBox->setFixedWidth(121);
// #else
//     typeComboBox->setFixedWidth(120);
// #endif

    // typeComboBox->setMaximumHeight(32);

    // typeComboBox->addItem(tr("All Type"), TransactionFilterProxy::ALL_TYPES);
    // typeComboBox->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
    //                                     TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    // typeComboBox->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
    //                               TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    // typeComboBox->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    // typeComboBox->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    // typeComboBox->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));



    transactionView->setSearchWidget(dateComboBox,typeComboBox,addressEdit);

    connect(typeComboBox, SIGNAL(activated(int)), transactionView, SLOT(chooseType(int)));
    connect(dateComboBox, SIGNAL(activated(int)), transactionView, SLOT(chooseDate(int)));
    connect(addressEdit, SIGNAL(textChanged(QString)), transactionView, SLOT(changedPrefix(QString)));

    QLabel* label_2 = new QLabel(this);
    // label_2->setFixedWidth(60);

    label_2->setMinimumSize(160,10);
    label_2->setMaximumSize(160,10);

    exportButton->setMinimumSize(80,30);
    exportButton->setMaximumSize(80,30);
    

    exportButton->setIcon(QIcon(":/pic/res/pic/outputData.png"));

#ifndef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    exportButton->setIcon(QIcon(":/pic/res/pic/outputData.png"));
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

    receiveCoinsPage = new ReceiveCoinsDialog();
    sendCoinsPage = new SendCoinsDialog();

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);

    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
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
        connect(overviewPage,SIGNAL(updateBalance(QString ,QString ,QString )),gui,SIGNAL(updateBalance(QString,QString,QString)));

        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(encryptionStatusChanged(int)), gui, SLOT(setEncryptionStatus(int)));

        // Pass through transaction notifications
        connect(this, SIGNAL(incomingTransaction(QString,int,CAmount,QString,QString)), gui, SLOT(incomingTransaction(QString,int,CAmount,QString,QString)));
    }
}

void WalletView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    overviewPage->setClientModel(clientModel);
    sendCoinsPage->setClientModel(clientModel);
}

void WalletView::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;

    // Put transaction list in tabs
    transactionView->setModel(walletModel);
    overviewPage->setWalletModel(walletModel);
    receiveCoinsPage->setModel(walletModel);
    sendCoinsPage->setModel(walletModel);

    if (walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));

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
    QString address = ttm->index(start, TransactionTableModel::ToAddress, parent).data().toString();

    emit incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(sendCoinsPage);

    if (!addr.isEmpty())
        sendCoinsPage->setAddress(addr);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(0);
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
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(0);
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
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    emit encryptionStatusChanged(walletModel->getEncryptionStatus());
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, 0);


    dlg.setModel(walletModel);

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();

    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);


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
        emit message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        emit message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, 0);
    dlg.setModel(walletModel);

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);

    dlg.exec();
}

void WalletView::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, 0);
        dlg.setModel(walletModel);

        QPoint pos = MassGridGUI::winPos();
        QSize size = MassGridGUI::winSize();
        dlg.move(pos.x()+(size.width()-dlg.width())/2,pos.y()+(size.height()-dlg.height())/2);

        dlg.exec();
    }
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab, 0);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg->move(pos.x()+(size.width()-dlg->width())/2,pos.y()+(size.height()-dlg->height())/2);

    dlg->show();
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, 0);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());
    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dlg->move(pos.x()+(size.width()-dlg->width())/2,pos.y()+(size.height()-dlg->height())/2);

    dlg->show();
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
