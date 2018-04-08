#ifndef PRIVKEYMGR_H
#define PRIVKEYMGR_H

#include <QWidget>
#include <QDialog>
#include <QTimer>

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

private slots:
    void on_okButton_clicked();

    void on_stackedWidget_currentChanged(int arg1);

    void slot_RunCMD();

    void slot_timeOut();

private:
    Ui::PrivKeyMgr *ui;

    bool inputMode;

    MassGridGUI *m_guiObj;

    void changeCurrentPage(int index);
};

#endif // PRIVKEYMGR_H
