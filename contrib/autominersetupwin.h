#ifndef AUTOMINERSETUPWIN_H
#define AUTOMINERSETUPWIN_H

#include <QDialog>

namespace Ui {
class AutoMinerSetupWin;
}

class AutoMinerSetupWin : public QDialog
{
    Q_OBJECT

public:
    explicit AutoMinerSetupWin(QWidget *parent = 0);
    ~AutoMinerSetupWin();

private:
    Ui::AutoMinerSetupWin *ui;
};

#endif // AUTOMINERSETUPWIN_H
