#ifndef MAINWINTITLE_H
#define MAINWINTITLE_H

#include <QWidget>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QMouseEvent>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QResizeEvent>
#include <QLabel>

namespace Ui {
class MainwinTitle;
}

class MainwinTitle : public QWidget
{
    Q_OBJECT

public:
    explicit MainwinTitle(QWidget *parent = 0);
    ~MainwinTitle();

    bool pressFlag();

    QMenu* fileMenu(const QString& text);
    QMenu* settingsMenu(const QString& text);
    QMenu* helpMenu(const QString& text);

    void setButtonText(const QString& overview,const QString& send,const QString& Transactions);

private:
    Ui::MainwinTitle *ui; 

    bool m_mousePress;

    QMenu *m_fileMenu;
    QMenu *m_settingsMenu;
    QMenu *m_helpMenu;

    void setLabelStyle(QLabel* label);

protected:
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);

signals:
    void sgl_close();
    void sgl_showMin();
    void sgl_showMax();
    void sgl_showOverview();
    void sgl_showSendPage();
    void sgl_showExchangePage();

private slots:
    void updateBalance(QString balance,QString unconfirmed,QString total);
    void openFileMenu();
    void openSettingsMenu();
    void openHelpMenu();
    void on_toolButton_clicked();
    void on_toolButton_2_clicked();
    void on_toolButton_3_clicked();
};

#endif // MAINWINTITLE_H
