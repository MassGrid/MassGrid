#ifndef LOADINGWIN_H
#define LOADINGWIN_H

#include <QWidget>
// #include <QMovie>

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
    
private:
    Ui::LoadingWin *ui;

    // QMovie *m_movie;

};

#endif // LOADINGWIN_H
