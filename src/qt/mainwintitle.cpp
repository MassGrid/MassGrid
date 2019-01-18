#include "mainwintitle.h"
#include "ui_mainwintitle.h"
#include <QSortFilterProxyModel>
#include "addresstablemodel.h"
#include "receiverequestdialog.h"
#include "editaddressdialog.h"
#include "massgridgui.h"
#include "wallet/wallet.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "util.h"
#include <QSettings>

#ifdef ENABLE_WALLET
#include "walletmodel.h"
#endif // ENABLE_WALLET

MainwinTitle::MainwinTitle(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainwinTitle)
    ,m_mousePress(false)
    ,m_fileMenu(0)
    ,m_settingsMenu(0)
    ,m_helpMenu(0)
    ,m_mapper(0)

{
    ui->setupUi(this);
    connect(ui->pBtnMin,SIGNAL(clicked()),this,SIGNAL(sgl_showMin()));
    connect(ui->pBtnMax,SIGNAL(clicked()),this,SIGNAL(sgl_showMax()));
    connect(ui->pBtnClose,SIGNAL(clicked()),this,SIGNAL(sgl_close()));

    connect(ui->fileButton,SIGNAL(clicked()),this,SLOT(openFileMenu()));
    connect(ui->settingsButton,SIGNAL(clicked()),this,SLOT(openSettingsMenu()));
    connect(ui->helpButton,SIGNAL(clicked()),this,SLOT(openHelpMenu()));
    connect(ui->openAddr,SIGNAL(clicked()),this,SLOT(open2DCodePage()));

	setLabelStyle(ui->label_overview);

    m_mapper = new QDataWidgetMapper(this);
    m_mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

#ifdef Q_OS_MAC
    ui->openAddr->hide();
    ui->pBtnMin->hide();
#endif

#ifdef Q_OS_WIN
    QFont font = ui->fileButton->font();
    font.setPointSize(12);

    ui->fileButton->setFont(font);
    ui->settingsButton->setFont(font);
    ui->helpButton->setFont(font);
#endif
    initMasternode();
}

MainwinTitle::~MainwinTitle()
{
    delete ui;
}

void MainwinTitle::mouseReleaseEvent(QMouseEvent *e)
{
	m_mousePress = false;
    QWidget::mouseReleaseEvent(e);
}

void MainwinTitle::mousePressEvent(QMouseEvent *e)
{
	m_mousePress = true;
    QWidget::mousePressEvent(e);
}

bool MainwinTitle::pressFlag()
{
	return m_mousePress;
}

void MainwinTitle::updateBalance(QString balance,QString unconfirmed,QString immature,bool showImmature,bool showWatchOnlyImmature,QString total)
{
	ui->label_balance->setText(balance);
	ui->label_unconfirmed->setText(unconfirmed);
    ui->labelWatchImmature->setText(immature);

    ui->labelWatchImmature->setVisible(showImmature||showWatchOnlyImmature); // show watch-only immature balance
    ui->labelmmatureText->setVisible(showImmature||showWatchOnlyImmature);
	ui->label_total->setText(total);
}

QMenu* MainwinTitle::fileMenu(const QString& text)
{
	 if(!m_fileMenu){
	 	m_fileMenu = new QMenu();
	 	ui->fileButton->setText(text);
	 }
	 return m_fileMenu;  
}

QMenu* MainwinTitle::settingsMenu(const QString& text)
{
	 if(!m_settingsMenu){
	 	m_settingsMenu = new QMenu();
	 	ui->settingsButton->setText(text);
	 }
	 return m_settingsMenu;
}

QMenu* MainwinTitle::helpMenu(const QString& text)
{
	 if(!m_helpMenu){
	 	m_helpMenu = new QMenu();
	 	ui->helpButton->setText(text);
	 }
	 return m_helpMenu;
}

void MainwinTitle::openFileMenu()
{
	if(!m_fileMenu)
		return ;
    QPoint point = ui->fileButton->mapToGlobal(ui->fileButton->pos());
    QPoint menuPos(point.x()-ui->fileButton->pos().x(),point.y()+ui->fileButton->height());
    m_fileMenu->exec(menuPos);
}

void MainwinTitle::openSettingsMenu()
{
	if(!m_settingsMenu)
		return ;

    QPoint point = ui->settingsButton->mapToGlobal(ui->settingsButton->pos());
    QPoint menuPos(point.x()-ui->settingsButton->pos().x(),point.y()+ui->settingsButton->height());
    m_settingsMenu->exec(menuPos);

}
void MainwinTitle::openHelpMenu()
{
	if(!m_helpMenu)
		return ;
    QPoint point = ui->helpButton->mapToGlobal(ui->helpButton->pos());
    QPoint menuPos(point.x()-ui->helpButton->pos().x(),point.y()+ui->helpButton->height());
    m_helpMenu->exec(menuPos);
}

void MainwinTitle::setButtonText(const QString& overview,const QString& send,const QString& Transactions)
{
	ui->toolButton->setText(overview);
	ui->toolButton_2->setText(send);
	ui->transactionButton->setText(Transactions);
}

void MainwinTitle::on_toolButton_clicked()
{
	setLabelStyle(ui->label_overview);
	Q_EMIT sgl_showOverview();
}

void MainwinTitle::on_toolButton_2_clicked()
{
	setLabelStyle(ui->label_send);
	Q_EMIT sgl_showSendPage();
}

void MainwinTitle::on_transactionButton_clicked()
{
	setLabelStyle(ui->label_transactions);
	Q_EMIT sgl_showExchangePage();
}

void MainwinTitle::on_masternodeButton_clicked()
{
    setLabelStyle(ui->label_masternode);
    Q_EMIT sgl_showMasternodePage();
}

void MainwinTitle::setLabelStyle(QLabel* label)
{
	ui->label_transactions->setStyleSheet("");
	ui->label_send->setStyleSheet("");
	ui->label_overview->setStyleSheet("");
	ui->label_masternode->setStyleSheet("");

	label->setStyleSheet("image: url(:/res/pic/sjx.png);");
}

void MainwinTitle::setModel(WalletModel *model)
{
    this->walletModel = model;
    if(!model)
        return;

    AddressTableModel* addressModel = walletModel->getAddressTableModel();
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(addressModel);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Receive);

    // m_mapper->setModel(proxyModel);
    // // mapper->addMapping(ui->labelEdit, AddressTableModel::Label);
    // m_mapper->addMapping(ui->addressEdit, AddressTableModel::Address);

    OptionsModel* optionsmodel = walletModel->getOptionsModel();
    QString mainAddress = optionsmodel->getMainAddress();
    QString defaultAddressStr;

    if(!mainAddress.isEmpty()){
        int rowCount = proxyModel->rowCount();
        for(int i=0;i<rowCount;i++){
            QString address = proxyModel->index(i,1).data().toString();

            if(mainAddress.contains(address)){
                defaultAddressStr = mainAddress;
                break;
            }
        }
    }
    else{
        defaultAddressStr = proxyModel->index(0,1).data().toString();
    }
    ui->addressEdit->setText(defaultAddressStr);
    ui->addressEdit->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background-color: rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item:!enabled{\n    background:transparent;\n	color:rgb(125,125,125);\n}QMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n    margin-right: 2px;\n    margin-top: 2px;\n    margin-bottom: 2px;\n    height:30px;\n}\n\nQMenu::item:selected:enabled{\n    background-color: rgb(239,169,4);\n    color: white;\n}\n\nQMenu::item:selected:!enabled{\n    background:transparent;\n}\nQLineEdit{\nborder:hidden;\nbackground-color: rgba(255, 255, 255, 0);\ncolor: rgb(255, 255, 255);\n}");

    optionsmodel->setDefaultReceiveAddress(defaultAddressStr);
}

void MainwinTitle::open2DCodePage()
{
	QString label = "";
	QString reqMessage = "";
	CAmount mount = 0;
    SendCoinsRecipient info(ui->addressEdit->text(), label,mount,reqMessage);
    ReceiveRequestDialog dialog;
    dialog.setModel(walletModel->getOptionsModel());
    dialog.setInfo(info);

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dialog.move(pos.x()+(size.width()-dialog.width())/2,pos.y()+(size.height()-dialog.height())/2);
    dialog.exec();
}

// bool MassGridUnits::parse(int unit, const QString &value, CAmount *val_out)

CAmount MainwinTitle::getTotal()
{
    OptionsModel* optionsmodel = walletModel->getOptionsModel();
    int unit = optionsmodel->getDisplayUnit();

    QString total = ui->label_total->text().remove("\n");
    QString totalUnit = total.split(" ").last();
    total.replace(totalUnit,"");
    total.replace(" ","");

    CAmount mount = 0;
    MassGridUnits::parse(unit,total,&mount);

    return mount;
}

void MainwinTitle::loadRow(int row)
{
    m_mapper->setCurrentIndex(row);

    GUIUtil::setDefaultReceiveAddr(ui->addressEdit->text());
}

void MainwinTitle::setTitle(const QString& titleName)
{
    ui->label_title->setText(titleName);
}

void MainwinTitle::setTransactionButtonStyle()
{
    // on_transactionButton_clicked();
    setLabelStyle(ui->label_transactions);
}

void MainwinTitle::initMasternode()
{
    QSettings settings;

    // Window
    settings.setValue("fShowMasternodesTab", true);
    if (!settings.contains("fShowMasternodesTab"))
        settings.setValue("fShowMasternodesTab", true);
    bool fHideTrayIcon = settings.value("fShowMasternodesTab").toBool();
    fHideTrayIcon = true;
    if(!fHideTrayIcon){
        ui->masternodeButton->hide();
        ui->label_masternode->hide();
    }
}
