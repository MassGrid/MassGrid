#ifndef PRIVKEYMGR_H
#define PRIVKEYMGR_H

#include <QWidget>
#include <QDialog>

namespace Ui {
class PrivKeyMgr;
}

class PrivKeyMgr : public QDialog
{
    Q_OBJECT

public:
    explicit PrivKeyMgr(bool inputMode,QWidget *parent = 0);
    ~PrivKeyMgr();

private slots:
    void on_okButton_clicked();

    void on_stackedWidget_currentChanged(int arg1);

private:
    Ui::PrivKeyMgr *ui;

    bool inputMode;

    void changeCurrentPage(int index);
};

#endif // PRIVKEYMGR_H
