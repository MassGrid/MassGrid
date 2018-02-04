#include "mainwintitle.h"
#include "ui_mainwintitle.h"
#include <QMessageBox>

MainwinTitle::MainwinTitle(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainwinTitle)
    ,m_mousePress(false)
    ,m_fileMenu(0)
    ,m_settingsMenu(0)
    ,m_helpMenu(0)
{
    ui->setupUi(this);
    connect(ui->pushButton_3,SIGNAL(clicked()),this,SIGNAL(sgl_showMin()));
    connect(ui->pushButton,SIGNAL(clicked()),this,SIGNAL(sgl_close()));
    // connect(ui->toolButton,SIGNAL(clicked()),this,SIGNAL(sgl_showOverview()));
    // connect(ui->toolButton_2,SIGNAL(clicked()),this,SIGNAL(sgl_showSendPage()));
    // connect(ui->toolButton_3,SIGNAL(clicked()),this,SIGNAL(sgl_showExchangePage()));

    connect(ui->fileButton,SIGNAL(clicked()),this,SLOT(openFileMenu()));
    connect(ui->settingsButton,SIGNAL(clicked()),this,SLOT(openSettingsMenu()));
    connect(ui->helpButton,SIGNAL(clicked()),this,SLOT(openHelpMenu()));

	setLabelStyle(ui->label_overview);

    // connect(ui->toolButton_5,SIGNAL(clicked()),this,SLOT(openFileMenu()()()));
    // connect(ui->toolButton_6,SIGNAL(clicked()),this,SLOT(openSettingsMenu()()));
    // connect(ui->toolButton_7,SIGNAL(clicked()),this,SLOT(openHelpMenu()()));
    // connect(ui->toolButton_4,SIGNAL(clicked()),this,SLOT(openFileMenu()));

    // connect(ui->toolButton_9,SIGNAL(clicked()),this,SLOT(openFileMenu()()()));
    // connect(ui->toolButton_4,SIGNAL(clicked()),this,SLOT(openSettingsMenu()()));
    // connect(ui->toolButton_8,SIGNAL(clicked()),this,SLOT(openHelpMenu()()));

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
 	    m_fileMenu->setStyleSheet(
                " QMenu {\
                color:rgb(255,255,255);\
                background-color: rgb(198,125,26); /* sets background of the menu 设置整个菜单区域的背景色，我用的是白色：white*/\
                /*border: 1px solid white;整个菜单区域的边框粗细、样式、颜色*/\
            }\
            QMenu::item {\
                min-width:120px;\
                min-height:30px;\
                color:rgb(255,255,255);\
                background-color: transparent;\
                padding:0px 0px 0px 20px;/*设置菜单项文字上下和左右的内边距，效果就是菜单中的条目左右上下有了间隔*/\
                margin:1px 1px;/*设置菜单项的外边距*/\
                /*border-bottom:1px solid #DBDBDB;为菜单项之间添加横线间隔*/\
            }\
            QMenu::item:selected { /* when user selects item using mouse or keyboard */\
            color:rgb(255,255,255);\
                background-color: rgb(239,169,4);/*这一句是设置菜单项鼠标经过选中的样式*/\
            }");
	 }
	 return m_fileMenu;
}

QMenu* MainwinTitle::settingsMenu(const QString& text)
{
	 if(!m_settingsMenu){
	 	m_settingsMenu = new QMenu();
	 	ui->settingsButton->setText(text);
 	    m_settingsMenu->setStyleSheet(
                " QMenu {\
                color:rgb(255,255,255);\
                background-color: rgb(198,125,26); /* sets background of the menu 设置整个菜单区域的背景色，我用的是白色：white*/\
                /*border: 1px solid white;整个菜单区域的边框粗细、样式、颜色*/\
            }\
            QMenu::item {\
                min-width:120px;\
                min-height:30px;\
                color:rgb(255,255,255);\
                background-color: transparent;\
                padding:0px 0px 0px 20px;/*设置菜单项文字上下和左右的内边距，效果就是菜单中的条目左右上下有了间隔*/\
                margin:1px 1px;/*设置菜单项的外边距*/\
                /*border-bottom:1px solid #DBDBDB;为菜单项之间添加横线间隔*/\
            }\
            QMenu::item:selected { /* when user selects item using mouse or keyboard */\
            color:rgb(255,255,255);\
                background-color: rgb(239,169,4);/*这一句是设置菜单项鼠标经过选中的样式*/\
            }");
	 }
	 return m_settingsMenu;
}

QMenu* MainwinTitle::helpMenu(const QString& text)
{
	 if(!m_helpMenu){
	 	m_helpMenu = new QMenu();
	 	ui->helpButton->setText(text);
 	    m_helpMenu->setStyleSheet(
                " QMenu {\
                color:rgb(255,255,255);\
                background-color: rgb(198,125,26); /* sets background of the menu 设置整个菜单区域的背景色，我用的是白色：white*/\
                /*border: 1px solid white;整个菜单区域的边框粗细、样式、颜色*/\
            }\
            QMenu::item {\
                min-width:120px;\
                min-height:30px;\
                color:rgb(255,255,255);\
                background-color: transparent;\
                padding:0px 0px 0px 20px;/*设置菜单项文字上下和左右的内边距，效果就是菜单中的条目左右上下有了间隔*/\
                margin:1px 1px;/*设置菜单项的外边距*/\
                /*border-bottom:1px solid #DBDBDB;为菜单项之间添加横线间隔*/\
            }\
            QMenu::item:selected { /* when user selects item using mouse or keyboard */\
            color:rgb(255,255,255);\
                background-color: rgb(239,169,4);/*这一句是设置菜单项鼠标经过选中的样式*/\
            }");
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