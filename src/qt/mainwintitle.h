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
#include <QDataWidgetMapper>

class AddressTableModel;
class WalletModel;

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
    void setModel(WalletModel *model);
    void loadRow(int row);
    void setTitle(const QString& titleName);

    // QString getReceiveAddr();
    void setTransactionButtonStyle();

private:
    Ui::MainwinTitle *ui; 

    bool m_mousePress;

    QMenu *m_fileMenu;
    QMenu *m_settingsMenu;
    QMenu *m_helpMenu;
    QDataWidgetMapper *m_mapper;
    WalletModel *walletModel;

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
    void sgl_open2DCodePage();

private slots:
    void updateBalance(QString balance,QString unconfirmed,QString immature,bool showImmature,bool showWatchOnlyImmature,QString total);
    void openFileMenu();
    void openSettingsMenu();
    void openHelpMenu();
    void open2DCodePage();
    void on_toolButton_clicked();
    void on_toolButton_2_clicked();
    // void on_toolButton_3_clicked();
    void on_transactionButton_clicked();
};

#endif // MAINWINTITLE_H
