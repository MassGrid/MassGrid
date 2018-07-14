#ifndef PRIVKEYMGR_H
#define PRIVKEYMGR_H

#include <QWidget>
#include <QDialog>
#include <QTimer>
#include <QMouseEvent>

class MassGridGUI;

namespace Ui {
class PrivKeyMgr;
}

class PrivKeyMgr : public QDialog
{
    Q_OBJECT

public:
    explicit PrivKeyMgr(bool inputMode,MassGridGUI *parentObj,QWidget *parent = 0);
    ~PrivKeyMgr();

// signals:
//     void sgl_startToImportPrivkey(QString,int);

private Q_SLOTS:
    void on_okButton_clicked();

    void on_stackedWidget_currentChanged(int arg1);

    void slot_RunCMD();

    void slot_timeOut();

private:
    Ui::PrivKeyMgr *ui;

    bool m_inputMode;

    MassGridGUI *m_guiObj;
    bool m_mousePress;
    QPoint m_last;

    void changeCurrentPage(int index);

private:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // PRIVKEYMGR_H
