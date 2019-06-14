// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "receiverequestdialog.h"
#include "ui_receiverequestdialog.h"

#include "massgridunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QClipboard>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#if defined(HAVE_CONFIG_H)
#include "config/massgrid-config.h" /* for USE_QRCODE */
#endif

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

QRImageWidget::QRImageWidget(QWidget *parent):
    QLabel(parent), contextMenu(0)
{
    contextMenu = new QMenu();
    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    contextMenu->addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(copyImage()));
    contextMenu->addAction(copyImageAction);
    contextMenu->setStyleSheet("QMenu{\ncolor:rgb(255,255,255);\n    background:rgb(198,125,26);\n    border:0px solid transparent;\n}\nQMenu::item{\n    padding:0px 20px 0px 20px;\n    margin-left: 2px;\n  margin-right: 2px;\n    margin-top: 2px;\n  margin-bottom: 2px;\n    height:30px;\n}\n \nQMenu::item:!enabled{\n    background:transparent;\n	color:rgb(125,125,125);\n}QMenu::item:selected:enabled{\n    background-color: rgb(239,169,4); \n    color: white;            \n}\n \nQMenu::item:selected:!enabled{\n    background:transparent;\n}");
}

QImage QRImageWidget::exportImage()
{
    if(!pixmap())
        return QImage();
    return pixmap()->toImage().scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE);
}

void QRImageWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && pixmap())
    {
        event->accept();
        QMimeData *mimeData = new QMimeData;
        mimeData->setImageData(exportImage());

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec();
    } else {
        QLabel::mousePressEvent(event);
    }
}

void QRImageWidget::saveImage()
{
    if(!pixmap())
        return;
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Image (*.png)"), NULL);
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void QRImageWidget::copyImage()
{
    if(!pixmap())
        return;
    QApplication::clipboard()->setImage(exportImage());
}

void QRImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if(!pixmap())
        return;
    contextMenu->exec(event->globalPos());
}

ReceiveRequestDialog::ReceiveRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveRequestDialog),
    model(0),
    m_mousePress(false)
{
    ui->setupUi(this);

#ifndef USE_QRCODE
    // ui->btnSaveAs->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    // connect(ui->btnSaveAs, SIGNAL(clicked()), ui->lblQRCode, SLOT(saveImage()));
    setWindowFlags(Qt::FramelessWindowHint);
    connect(ui->cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    ui->label_titleName->setText(this->windowTitle());
    this->setAttribute(Qt::WA_TranslucentBackground);

    GUIUtil::MakeShadowEffect(this,ui->centerWin);
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    delete ui;
}

void ReceiveRequestDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(update()));

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &info)
{
    this->info = info;
    update();
}

void ReceiveRequestDialog::update()
{
    if(!model)
        return;
    QString target = info.label;
    if(target.isEmpty())
        target = info.address;
    setWindowTitle(tr("Request payment to %1").arg(target));

    // QString uri = GUIUtil::formatMassGridURI(info);
    QString uri = info.address;
    // ui->btnSaveAs->setEnabled(false);
    // QString html;
    // html += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    // html += "<b>"+tr("Payment information")+"</b><br>";
    // html += "<b>"+tr("URI")+"</b>: ";
    // html += "<a href=\""+uri+"\">" + GUIUtil::HtmlEscape(uri) + "</a><br>";
    // html += "<b>"+tr("Address")+"</b>: " + GUIUtil::HtmlEscape(info.address) + "<br>";
    // if(info.amount)
    //     html += "<b>"+tr("Amount")+"</b>: " + MassGridUnits::formatHtmlWithUnit(model->getDisplayUnit(), info.amount) + "<br>";
    // if(!info.label.isEmpty())
    //     html += "<b>"+tr("Label")+"</b>: " + GUIUtil::HtmlEscape(info.label) + "<br>";
    // if(!info.message.isEmpty())
    //     html += "<b>"+tr("Message")+"</b>: " + GUIUtil::HtmlEscape(info.message) + "<br>";
    // html += "<b>"+tr("InstantSend")+"</b>: " + (info.fUseInstantSend ? tr("Yes") : tr("No")) + "<br>";
    // ui->outUri->setText(html);

#ifdef USE_QRCODE
    ui->lblQRCode->setText("");
    if(!uri.isEmpty())
    {
        // limit URI length
        if (uri.length() > MAX_URI_LENGTH)
        {
            ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        } else {
            QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
            if (!code)
            {
                ui->lblQRCode->setText(tr("Error encoding URI into QR Code."));
                return;
            }
            QImage myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
            myImage.fill(0xffffff);
            unsigned char *p = code->data;
            for (int y = 0; y < code->width; y++)
            {
                for (int x = 0; x < code->width; x++)
                {
                    myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                    p++;
                }
            }
            QRcode_free(code);

            ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));
            // ui->btnSaveAs->setEnabled(true);
        }
    }
#endif
}

// void ReceiveRequestDialog::on_btnCopyURI_clicked()
// {
//     GUIUtil::setClipboard(GUIUtil::formatMassGridURI(info));
// }

// void ReceiveRequestDialog::on_btnCopyAddress_clicked()
// {
//     GUIUtil::setClipboard(info.address);
// }

void ReceiveRequestDialog::mousePressEvent(QMouseEvent *e)
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

void ReceiveRequestDialog::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    m_last = e->globalPos();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}

void ReceiveRequestDialog::mouseReleaseEvent(QMouseEvent *e)
{
    if(!m_mousePress)
        return ;
    m_mousePress = false;
    int dx = e->globalX() - m_last.x();
    int dy = e->globalY() - m_last.y();
    this->move(QPoint(this->x()+dx, this->y()+dy));
}
