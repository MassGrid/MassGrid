// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_QT_UTILITYDIALOG_H
#define MASSGRID_QT_UTILITYDIALOG_H

#include <QDialog>
#include <QObject>

class MassGridGUI;
class ClientModel;

namespace Ui {
    class HelpMessageDialog;
}

/** "Help message" dialog box */
class HelpMessageDialog : public QDialog
{
    Q_OBJECT

public:
    enum HelpMode {
        about,
        cmdline,
        pshelp
    };

    explicit HelpMessageDialog(QWidget *parent, HelpMode helpMode);
    ~HelpMessageDialog();

    void printToConsole();
    void showOrPrint();

private:
    Ui::HelpMessageDialog *ui;
    QString text;

private Q_SLOTS:
    void on_okButton_accepted();
private:
    QPoint m_last;
    bool m_mousePress;
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};


/** "Shutdown" window */
class ShutdownWindow : public QWidget
{
    Q_OBJECT

public:
    ShutdownWindow(QWidget *parent=0, Qt::WindowFlags f=0);
    static QWidget *showShutdownWindow(MassGridGUI *window);

protected:
    void closeEvent(QCloseEvent *event);
};

#endif // MASSGRID_QT_UTILITYDIALOG_H
