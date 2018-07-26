#include "cprogressdialog.h"
#include "ui_cprogressdialog.h"

CProgressDialog::CProgressDialog(const QString &labelText, int minimum, int maximum, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CProgressDialog),
    m_mousePress(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    ui->label_titleName->setText(labelText);
    this->setAttribute(Qt::WA_TranslucentBackground);

    ui->progressBar->setMinimum(minimum);
    ui->progressBar->setMaximum(maximum);
}

CProgressDialog::~CProgressDialog()
{
    delete ui;
}

void CProgressDialog::setValue(int value)
{
    ui->progressBar->setValue(value);
}

void CProgressDialog::mousePressEvent(QMouseEvent *e)
{
    int posx = e->pos().x();
    int posy = e->pos().y();
    int framex = ui->mainframe->pos().x();
    int framey = ui->mainframe->pos().y();
    int frameendx = framex+ui->mainframe->width();
    int frameendy = framey+30;
    if(posx>framex && posx<frameendx && posy>framey && posy<frameendy){
        m_mousePress = true;
        m_last = e->globalPos();
    }
    else{
        m_mousePress = false;
    }
}

void CProgressDialog::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void CProgressDialog::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}
