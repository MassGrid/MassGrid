#include "autominersetupwin.h"
#include "ui_autominersetupwin.h"
#include "guiutil.h"
#include "util.h"

AutoMinerSetupWin::AutoMinerSetupWin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoMinerSetupWin),
    m_mousePress(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->pBtn_ok,SIGNAL(clicked()),this,SLOT(slot_confirmSetup()));
    connect(ui->pBtn_defaultSetup,SIGNAL(clicked()),this,SLOT(slot_fillAutoMinerSetup()));
    connect(ui->pBtn_clear,SIGNAL(clicked()),this,SLOT(slot_clearSetup()));
    connect(ui->pBtn_customMode,SIGNAL(clicked()),this,SLOT(onStackWidgetPageChanged()));
    connect(ui->pBtn_defaultMode,SIGNAL(clicked()),this,SLOT(onStackWidgetPageChanged()));
    connect(ui->pBtn_add,SIGNAL(clicked()),this,SLOT(onAddPathRow()));
    connect(ui->pBtn_delete,SIGNAL(clicked()),this,SLOT(onRemovePathRow()));

    connect(ui->tableWidget,SIGNAL(clicked(QModelIndex)),this,SLOT(ontableWidgetClicked(QModelIndex)));
    
    this->setAttribute(Qt::WA_TranslucentBackground);
    inittableWidget();
    GUIUtil::MakeShadowEffect(this,ui->centerWin);
    ui->stackedWidget->setCurrentWidget(ui->defaultModePage);   

}

AutoMinerSetupWin::~AutoMinerSetupWin()
{
    delete ui;
}

void AutoMinerSetupWin::slot_fillAutoMinerSetup()
{
    ui->lineEdit_worker->setText(ui->lineEdit_worker->placeholderText());
    ui->lineEdit_minerpool->setText(ui->lineEdit_minerpool->placeholderText());
    ui->lineEdit_minertype->setText(ui->lineEdit_minertype->placeholderText());
}

void AutoMinerSetupWin::slot_clearSetup()
{
    if(ui->stackedWidget->currentWidget() == ui->defaultModePage){
        ui->lineEdit_worker->setText("");
        ui->lineEdit_minerpool->setText("");
        ui->lineEdit_minertype->setText("");
        ui->lineEdit_address->setText("");
    }
    else if(ui->stackedWidget->currentWidget() == ui->customModePage){
        int rowCount = ui->tableWidget->rowCount();

        for(int i=0;i<rowCount;i++){
            ui->tableWidget->removeRow(0);
        }
    }
}

void AutoMinerSetupWin::slot_confirmSetup()
{
    if(ui->stackedWidget->currentWidget() == ui->defaultModePage){
        if(ui->lineEdit_address->text().isEmpty()){
            CMessageBox::information(this, tr("Setup error"),tr("Miner address is empty!"));
            return ;
        }
        else if(ui->lineEdit_worker->text().isEmpty()){
            CMessageBox::information(this, tr("Setup error"),tr("Miner worker is empty!"));
            return ;
        }
        else if(ui->lineEdit_minerpool->text().isEmpty()){
            CMessageBox::information(this, tr("Setup error"),tr("Miner pool is empty!"));
            return ;
        }
        else if(ui->lineEdit_minertype->text().isEmpty()){
            CMessageBox::information(this, tr("Setup error"),tr("Miner type is empty!"));
            return;
        }
        std::map<std::string,std::string> env{};

        m_env["MINER_ADDRESS"] = ui->lineEdit_address->text().toStdString();
        m_env["MINER_WORKER"] = ui->lineEdit_worker->text().toStdString();
        m_env["MINER_POOL"] = ui->lineEdit_minerpool->text().toStdString();
        m_env["MINER_TYPE"] = ui->lineEdit_minertype->text().toStdString();
    }
    else if(ui->stackedWidget->currentWidget() == ui->customModePage){

        ui->tableWidget->setCurrentItem(NULL);
        int rowCount = ui->tableWidget->rowCount();
        int colCounmt = ui->tableWidget->columnCount();

        m_env.clear();
        for(int i=0;i<rowCount;i++){
            for(int j=0;j<colCounmt;j++){
                if(ui->tableWidget->item(i,j)->text().isEmpty()){
                    ui->tableWidget->item(i,j)->setBackgroundColor(Qt::red);
                    return;
                }
                else
                    ui->tableWidget->item(i,j)->setBackgroundColor(Qt::white);
            }

            m_env[ui->tableWidget->item(i,0)->text().toStdString()] = ui->tableWidget->item(i,1)->text().toStdString();
        }
    }

    accept();
}

std::map<std::string,std::string> AutoMinerSetupWin::getEnvSetup()
{
    return m_env;
}

void AutoMinerSetupWin::inittableWidget()
{
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    ui->tableWidget->setColumnWidth(0, 170*GUIUtil::GetDPIValue());

    ui->tableWidget->setFocus();
    ui->tableWidget->installEventFilter(this);
}

bool AutoMinerSetupWin::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->tableWidget){
        if(event->type() == QEvent::Leave){
            // ui->pBtn_add->setFocus();
            // ui->tableWidget->setFocus();
            // ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
            // ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
            // ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        }
        else if(event->type() == QEvent::Enter){
            // ui->tableWidget->setEditTriggers(!QAbstractItemView::NoEditTriggers);
        }
    }
    return QDialog::eventFilter(watched,event);     
}

void AutoMinerSetupWin::onAddPathRow()
{
    int rowCount = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(rowCount+1);
    ui->tableWidget->setItem(rowCount,0,new QTableWidgetItem(""));
    ui->tableWidget->setItem(rowCount,1,new QTableWidgetItem(""));

    ui->tableWidget->setRowHeight(ui->tableWidget->rowCount()-1,35);
}

void AutoMinerSetupWin::onRemovePathRow()
{
    int row = ui->tableWidget->currentRow();
    if(row < 0){
        ui->pBtn_delete->setEnabled(false);
        return ;
    }
    ui->tableWidget->removeRow(row);
    ui->pBtn_delete->setEnabled(false);
}

void AutoMinerSetupWin::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableWidget->indexAt(point);
    if(!index.isValid())
        return ;

    contextMenu->exec(QCursor::pos());
}

void AutoMinerSetupWin::mousePressEvent(QMouseEvent *e)
{
    int posx = e->pos().x();
    int posy = e->pos().y();
    int framex = ui->mainframe->pos().x();
    int framey = ui->mainframe->pos().y();
    int frameendx = framex+ui->mainframe->width();
    int frameendy = framey+30*GUIUtil::GetDPIValue();
    if(posx>framex && posx<frameendx && posy>framey && posy<frameendy){
        m_mousePress = true;
        m_last = e->globalPos();
    }
    else{
        m_mousePress = false;
    }
}

void AutoMinerSetupWin::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void AutoMinerSetupWin::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void AutoMinerSetupWin::onStackWidgetPageChanged()
{
    if(ui->pBtn_defaultMode->isChecked()){
        ui->stackedWidget->setCurrentWidget(ui->defaultModePage);
    }
    else if(ui->pBtn_customMode->isChecked()){
        ui->stackedWidget->setCurrentWidget(ui->customModePage);
    }
}

void AutoMinerSetupWin::ontableWidgetClicked(const QModelIndex &index)
{
    ui->pBtn_delete->setEnabled(true);
    QTableWidgetItem* item = ui->tableWidget->currentItem();
    ui->tableWidget->editItem(item);
}