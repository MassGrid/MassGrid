#ifndef AUTOMINERSETUPWIN_H
#define AUTOMINERSETUPWIN_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class AutoMinerSetupWin;
}

class AutoMinerSetupWin : public QDialog
{
    Q_OBJECT

public:
    explicit AutoMinerSetupWin(QWidget *parent = 0);
    ~AutoMinerSetupWin();

    std::map<std::string,std::string> getEnvSetup();

private:
    Ui::AutoMinerSetupWin *ui;

    QPoint m_last;
    bool m_mousePress;
    std::map<std::string,std::string> m_env;

    // void inittableWidget();
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private Q_SLOTS:
    void slot_fillAutoMinerSetup();
    void slot_confirmSetup();
    void slot_clearSetup();
    // void slot_addPathRow();
};

#endif // AUTOMINERSETUPWIN_H
