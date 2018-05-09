// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_SIGNVERIFYMESSAGEDIALOG_H
#define MASSGRID_QT_SIGNVERIFYMESSAGEDIALOG_H

#include <QDialog>
// #include <QObject>
#include <QMouseEvent>

class WalletModel;
// class QMouseEvent;

namespace Ui {
    class SignVerifyMessageDialog;
}

class SignVerifyMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignVerifyMessageDialog(QWidget *parent);
    ~SignVerifyMessageDialog();

    void setModel(WalletModel *model);
    void setAddress_SM(const QString &address);
    void setAddress_VM(const QString &address);

    void showTab_SM(bool fShow);
    void showTab_VM(bool fShow);

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::SignVerifyMessageDialog *ui;
    WalletModel *model;

    QPoint m_last;
    bool m_mousePress;
private slots:
    /* sign message */
    void on_addressBookButton_SM_clicked();
    void on_pasteButton_SM_clicked();
    void on_signMessageButton_SM_clicked();
    void on_copySignatureButton_SM_clicked();
    void on_clearButton_SM_clicked();
    /* verify message */
    void on_addressBookButton_VM_clicked();
    void on_verifyMessageButton_VM_clicked();
    void on_clearButton_VM_clicked();


protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // MASSGRID_QT_SIGNVERIFYMESSAGEDIALOG_H
