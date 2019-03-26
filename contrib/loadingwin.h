#ifndef LOADINGWIN_H
#define LOADINGWIN_H

#include <QWidget>

namespace Ui {
class LoadingWin;
}

class LoadingWin : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingWin(QWidget *parent = 0);
    ~LoadingWin();

private:
    Ui::LoadingWin *ui;
};

#endif // LOADINGWIN_H
