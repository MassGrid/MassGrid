#include "cprogressdialog.h"
#include "ui_cprogressdialog.h"

CProgressDialog::CProgressDialog(const QString &labelText, int minimum, int maximum, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CProgressDialog)
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
    m_last = e->globalPos();
}

void CProgressDialog::mouseMoveEvent(QMouseEvent *e)
{
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void CProgressDialog::mouseReleaseEvent(QMouseEvent *e)
{
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}