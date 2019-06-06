#include "cmessagebox.h"
#include "ui_cmessagebox.h"
#include "massgridgui.h"
#include "util.h"
#include "guiutil.h"

CMessageBox::CMessageBox(QWidget *parent,QString title,QString text) :
    QDialog(0),
    ui(new Ui::CMessageBox),
    m_mousePress(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(slot_close()));
    connect(ui->okButton,SIGNAL(clicked()),this,SLOT(slot_ok()));
    connect(ui->okButton2,SIGNAL(clicked()),this,SLOT(slot_ok()));
    connect(ui->closeButton,SIGNAL(clicked()),this,SLOT(slot_close()));
    

    ui->helpMessageLabel->setTextFormat(Qt::RichText);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->label_titleName->setText(title);
    ui->helpMessageLabel->setText(text);
    ui->helpMessageLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    if(parent!=0){
        QPoint pos = MassGridGUI::winPos();
        QSize size = MassGridGUI::winSize();
        move(pos.x()+(size.width()-width()*GUIUtil::GetDPIValue())/2,pos.y()+(size.height()-height()*GUIUtil::GetDPIValue())/2);
    }
    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

CMessageBox::~CMessageBox()
{
    delete ui;
}

void CMessageBox::setmode(StandardButton mode)
{
    switch (mode) {
    case Ok:
    {
        ui->closeButton->setVisible(false);
        ui->okButton->setVisible(false);
    }
        break;
    case Cancel:
        ui->okButton->setVisible(false);
        ui->okButton2->setVisible(false);
        break;
    case Ok_Cancel:
        ui->okButton2->setVisible(false);
        break;
    default:
        break;
    }
}

CMessageBox::StandardButton CMessageBox::information(QWidget *parent, const QString &title,
                                   const QString &text, StandardButton buttons,
                                   StandardButton defaultButton)
{
    CMessageBox dlg(parent,title,text);
    dlg.setmode(buttons);

    if(dlg.exec() == QDialog::Accepted){
        return Ok;
    }
    else {
        return Cancel;
    }
}

CMessageBox::StandardButton CMessageBox::critical(QWidget *parent, const QString &title,
     const QString &text, StandardButton buttons,
     StandardButton defaultButton)
{
    CMessageBox dlg(parent,title,text);
    dlg.setmode(buttons);

    if(dlg.exec() == QDialog::Accepted){
        return Ok;
    }
    else {
        return Cancel;
    }
}

CMessageBox::StandardButton CMessageBox::question(QWidget *parent, const QString &title,
     const QString &text, StandardButton buttons,StandardButton defaultButton)
{
    CMessageBox dlg(parent,title,text);
    dlg.setmode(buttons);

    if(dlg.exec() == QDialog::Accepted){
        return Ok;
    }
    else {
        return Cancel;
    }
}

CMessageBox::StandardButton CMessageBox::warning(QWidget *parent, const QString &title,
     const QString &text, StandardButton buttons,StandardButton defaultButton)
{
    CMessageBox dlg(parent,title,text);
    dlg.setmode(buttons);

    if(dlg.exec() == QDialog::Accepted){
        return Ok;
    }
    else {
        return Cancel;
    }
}


void CMessageBox::mousePressEvent(QMouseEvent *e)
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

void CMessageBox::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void CMessageBox::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void CMessageBox::slot_close()
{
    reject();
}
void CMessageBox::slot_ok()
{
    accept();
}