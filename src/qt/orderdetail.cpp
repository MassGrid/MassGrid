#include "orderdetail.h"
#include "ui_orderdetail.h"
#include "guiutil.h"

OrderDetail::OrderDetail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OrderDetail)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));

    ui->label_titlename->setText(this->windowTitle());
    this->setAttribute(Qt::WA_TranslucentBackground);
    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

OrderDetail::~OrderDetail()
{
    delete ui;
}

void OrderDetail::mousePressEvent(QMouseEvent *e)
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

void OrderDetail::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void OrderDetail::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}