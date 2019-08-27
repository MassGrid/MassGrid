#ifndef SIMPLESENDCOINDLG_H
#define SIMPLESENDCOINDLG_H

#include <QDialog>

namespace Ui {
class SimpleSendcoinDlg;
}

class SimpleSendcoinDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleSendcoinDlg(QWidget *parent = 0);
    ~SimpleSendcoinDlg();

private:
    Ui::SimpleSendcoinDlg *ui;
};

#endif // SIMPLESENDCOINDLG_H
