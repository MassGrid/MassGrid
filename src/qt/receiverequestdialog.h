// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_RECEIVEREQUESTDIALOG_H
#define MASSGRID_QT_RECEIVEREQUESTDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QImage>
#include <QLabel>

class OptionsModel;

namespace Ui {
    class ReceiveRequestDialog;
}

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class QRImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit QRImageWidget(QWidget *parent = 0);
    QImage exportImage();

public Q_SLOTS:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    QMenu *contextMenu;
};

class ReceiveRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveRequestDialog(QWidget *parent = 0);
    ~ReceiveRequestDialog();

    void setModel(OptionsModel *model);
    void setInfo(const SendCoinsRecipient &info);

private Q_SLOTS:
    void on_btnCopyURI_clicked();
    void on_btnCopyAddress_clicked();

    void update();

private:
    Ui::ReceiveRequestDialog *ui;
    OptionsModel *model;
    SendCoinsRecipient info;
    QPoint m_last;
    bool m_mousePress;
    
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // MASSGRID_QT_RECEIVEREQUESTDIALOG_H
