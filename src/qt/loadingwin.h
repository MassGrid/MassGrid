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

    void showLoading(const QString &msg);
    void hideWin();

    static void showLoading2(const QString& msg);
    static void hideLoadingWin();

private:
    Ui::LoadingWin *ui;

    
};

#endif // LOADINGWIN_H
