#include "loadingwin.h"
#include "ui_loadingwin.h"
#include "guiutil.h"
#include "massgridgui.h"

LoadingWin* g_loadingWin = NULL;

LoadingWin::LoadingWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadingWin)
{
    ui->setupUi(this);

    // m_movie = new QMovie("qrc:/res/icons/loading.gif");
    
    // m_movie->setScaledSize(ui->label->size());
    // ui->label->setMovie(m_movie);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating,true);

    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
}

LoadingWin::~LoadingWin()
{
    delete ui;
}

void LoadingWin::showLoading(const QString &msg)
{
    // m_movie->start();
    ui->label_msg->setText(msg);
    // setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    show();
}

void LoadingWin::hideWin()
{
    // m_movie->stop();
    setWindowFlags(windowFlags() &~ Qt::WindowStaysOnTopHint); 
    hide();
}

void LoadingWin::showLoading2(const QString& msg)
{
    if(g_loadingWin == NULL)
        g_loadingWin = new LoadingWin();
    
    QPoint pos = /*parent->pos();*/ MassGridGUI::winPos();
    QSize size = /*parent->size();*/ MassGridGUI::winSize();
    g_loadingWin->move(pos.x()+(size.width()-g_loadingWin->width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-g_loadingWin->height()*GUIUtil::GetDPIValue())/2);
    
    g_loadingWin->showLoading(msg);
}

void LoadingWin::hideLoadingWin()
{
    if(g_loadingWin != NULL){
        g_loadingWin->hideWin();
        delete g_loadingWin;
        g_loadingWin = NULL;
    }
}



// void AddDockerServiceDlg::showLoading(const QString & msg)
// {
//     if(m_loadingWin == NULL)
//         m_loadingWin = new LoadingWin(ui->centerWin);

//     QPoint pos = ui->centerWin->pos(); //MassGridGUI::winPos();
//     QSize size = ui->centerWin->size(); //MassGridGUI::winSize();
//     m_loadingWin->move(pos.x()+(size.width()-m_loadingWin->width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-m_loadingWin->height()*GUIUtil::GetDPIValue())/2);
    
//     m_loadingWin->showLoading(msg);
// }

// void AddDockerServiceDlg::hideLoadingWin()
// {
//     if(m_loadingWin != NULL){
//         m_loadingWin->hideWin();
//         delete m_loadingWin;
//         m_loadingWin = NULL;
//     }
// }