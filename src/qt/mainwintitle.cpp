#include "mainwintitle.h"
#include "ui_mainwintitle.h"
#include <QMessageBox>
#include "addresstablemodel.h"
#include "receiverequestdialog.h"
#include "editaddressdialog.h"
#include "massgridgui.h"

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
    // connect(ui->toolButton,SIGNAL(clicked()),this,SIGNAL(sgl_showOverview()));
    // connect(ui->toolButton_2,SIGNAL(clicked()),this,SIGNAL(sgl_showSendPage()));
    // connect(ui->toolButton_3,SIGNAL(clicked()),this,SIGNAL(sgl_showExchangePage()));

    connect(ui->fileButton,SIGNAL(clicked()),this,SLOT(openFileMenu()));
    connect(ui->settingsButton,SIGNAL(clicked()),this,SLOT(openSettingsMenu()));
    connect(ui->helpButton,SIGNAL(clicked()),this,SLOT(openHelpMenu()));
    connect(ui->openAddr,SIGNAL(clicked()),this,SLOT(open2DCodePage()));

	  setLabelStyle(ui->label_overview);

    m_mapper = new QDataWidgetMapper(this);
    m_mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

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

void MainwinTitle::updateBalance(QString balance,QString unconfirmed,QString total)
{
	ui->label_balance->setText(balance);
	ui->label_unconfirmed->setText(unconfirmed);
	ui->label_total->setText(total);
}

QMenu* MainwinTitle::fileMenu(const QString& text)
{
	 if(!m_fileMenu){
	 	m_fileMenu = new QMenu();
	 	ui->fileButton->setText(text);
 	    // m_fileMenu->setStyleSheet(
      //           " QMenu {\
      //           color:rgb(255,255,255);\
      //           background-color: rgb(198,125,26); /* sets background of the menu 设置整个菜单区域的背景色，我用的是白色：white*/\
      //           /*border: 1px solid white;整个菜单区域的边框粗细、样式、颜色*/\
      //       }\
      //       QMenu::item {\
      //           min-width:120px;\
      //           min-height:30px;\
      //           color:rgb(255,255,255);\
      //           background-color: transparent;\
      //           padding:0px 0px 0px 20px;/*设置菜单项文字上下和左右的内边距，效果就是菜单中的条目左右上下有了间隔*/\
      //           margin:1px 1px;/*设置菜单项的外边距*/\
      //           /*border-bottom:1px solid #DBDBDB;为菜单项之间添加横线间隔*/\
      //       }\
      //       QMenu::item:selected { /* when user selects item using mouse or keyboard */\
      //       color:rgb(255,255,255);\
      //           background-color: rgb(239,169,4);/*这一句是设置菜单项鼠标经过选中的样式*/\
      //       }");
        m_fileMenu->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background:rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n  margin-right: 2px;\n    margin-top: 2px;\n  margin-bottom: 2px;\n    height:30px;\n}\n \nQMenu::item:selected:enabled{\n    background-color: rgb(239,169,4); \n    color: white;            \n}\n \nQMenu::item:selected:!enabled{\n    background:transparent;\n}");

	 }
	 return m_fileMenu;  
}

QMenu* MainwinTitle::settingsMenu(const QString& text)
{
	 if(!m_settingsMenu){
	 	m_settingsMenu = new QMenu();
	 	ui->settingsButton->setText(text);
 	    // m_settingsMenu->setStyleSheet(
      //           " QMenu {\
      //           color:rgb(255,255,255);\
      //           background-color: rgb(198,125,26); /* sets background of the menu 设置整个菜单区域的背景色，我用的是白色：white*/\
      //           /*border: 1px solid white;整个菜单区域的边框粗细、样式、颜色*/\
      //       }\
      //       QMenu::item {\
      //           min-width:120px;\
      //           min-height:30px;\
      //           color:rgb(255,255,255);\
      //           background-color: transparent;\
      //           padding:0px 0px 0px 20px;/*设置菜单项文字上下和左右的内边距，效果就是菜单中的条目左右上下有了间隔*/\
      //           margin:1px 1px;/*设置菜单项的外边距*/\
      //           /*border-bottom:1px solid #DBDBDB;为菜单项之间添加横线间隔*/\
      //       }\
      //       QMenu::item:selected { /* when user selects item using mouse or keyboard */\
      //       color:rgb(255,255,255);\
      //           background-color: rgb(239,169,4);/*这一句是设置菜单项鼠标经过选中的样式*/\
      //       }");
        m_settingsMenu->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background:rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n  margin-right: 2px;\n    margin-top: 2px;\n  margin-bottom: 2px;\n    height:30px;\n}\n \nQMenu::item:selected:enabled{\n    background-color: rgb(239,169,4); \n    color: white;            \n}\n \nQMenu::item:selected:!enabled{\n    background:transparent;\n}");

	 }
	 return m_settingsMenu;
}

QMenu* MainwinTitle::helpMenu(const QString& text)
{
	 if(!m_helpMenu){
	 	m_helpMenu = new QMenu();
	 	ui->helpButton->setText(text);
 	    // m_helpMenu->setStyleSheet(
      //           " QMenu {\
      //           color:rgb(255,255,255);\
      //           background-color: rgb(198,125,26); /* sets background of the menu 设置整个菜单区域的背景色，我用的是白色：white*/\
      //           /*border: 1px solid white;整个菜单区域的边框粗细、样式、颜色*/\
      //       }\
      //       QMenu::item {\
      //           min-width:120px;\
      //           min-height:30px;\
      //           color:rgb(255,255,255);\
      //           background-color: transparent;\
      //           padding:0px 0px 0px 20px;/*设置菜单项文字上下和左右的内边距，效果就是菜单中的条目左右上下有了间隔*/\
      //           margin:1px 1px;/*设置菜单项的外边距*/\
      //           /*border-bottom:1px solid #DBDBDB;为菜单项之间添加横线间隔*/\
      //       }\
      //       QMenu::item:selected { /* when user selects item using mouse or keyboard */\
      //       color:rgb(255,255,255);\
      //           background-color: rgb(239,169,4);/*这一句是设置菜单项鼠标经过选中的样式*/\
      //       }");

        m_helpMenu->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background:rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n  margin-right: 2px;\n    margin-top: 2px;\n  margin-bottom: 2px;\n    height:30px;\n}\n \nQMenu::item:selected:enabled{\n    background-color: rgb(239,169,4); \n    color: white;            \n}\n \nQMenu::item:selected:!enabled{\n    background:transparent;\n}");

	 }
	 return m_helpMenu;
}

void MainwinTitle::openFileMenu()
{
	if(!m_fileMenu)
		return ;
    // QPoint point = ui->fileButton->mapToGlobal(ui->fileButton->pos());
    // QPoint menuPos(point.x()-ui->fileButton->pos().x(),point.y()+ui->fileButton->height());
    // m_fileMenu->exec(menuPos);

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
	ui->toolButton_3->setText(Transactions);
}


// label_transactions
// label_send
// label_overview
void MainwinTitle::on_toolButton_clicked()
{
	setLabelStyle(ui->label_overview);
	emit sgl_showOverview();
}

void MainwinTitle::on_toolButton_2_clicked()
{
	setLabelStyle(ui->label_send);
	emit sgl_showSendPage();
}

void MainwinTitle::on_toolButton_3_clicked()
{
	setLabelStyle(ui->label_transactions);
	emit sgl_showExchangePage();
}

void MainwinTitle::setLabelStyle(QLabel* label)
{
	ui->label_transactions->setStyleSheet("");
	ui->label_send->setStyleSheet("");
	ui->label_overview->setStyleSheet("");

	label->setStyleSheet("image: url(:/pic/res/pic/sjx.png);");
}

void MainwinTitle::setModel(WalletModel *model)
{
    this->walletModel = model;
    if(!model)
        return;

    m_mapper->setModel(walletModel->getAddressTableModel());
    // mapper->addMapping(ui->labelEdit, AddressTableModel::Label);
    m_mapper->addMapping(ui->addressEdit, AddressTableModel::Address);
}

    // SendCoinsRecipient info(address, label,
    //     ui->reqAmount->value(), ui->reqMessage->text());

void MainwinTitle::open2DCodePage()
{
	QString label = "";
	QString reqMessage = "";
	CAmount mount = 0;
    SendCoinsRecipient info(ui->addressEdit->text(), label,mount,reqMessage);
    // ReceiveRequestDialog *dialog = new ReceiveRequestDialog(0);
    // dialog->setAttribute(Qt::WA_DeleteOnClose);
    ReceiveRequestDialog dialog;
    dialog.setModel(walletModel->getOptionsModel());
    dialog.setInfo(info);

    QPoint pos = MassGridGUI::winPos();
    QSize size = MassGridGUI::winSize();
    dialog.move(pos.x()+(size.width()-dialog.width())/2,pos.y()+(size.height()-dialog.height())/2);
    dialog.exec();
}

void MainwinTitle::loadRow(int row)
{
    m_mapper->setCurrentIndex(row);
}

void MainwinTitle::setTitle(const QString& titleName)
{
    ui->label_title->setText(titleName);
}
