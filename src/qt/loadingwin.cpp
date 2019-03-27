#include "loadingwin.h"
#include "ui_loadingwin.h"

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

    // setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus); 
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
