#ifndef ADDDOCKERSERVICEDLG_H
#define ADDDOCKERSERVICEDLG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class AddDockerServiceDlg;
}

class WalletModel;

class AddDockerServiceDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AddDockerServiceDlg(QWidget *parent = nullptr);
    ~AddDockerServiceDlg();

    void setaddr_port(const std::string& addr_poprt);

    void setWalletModel(WalletModel* walletmodel);

private:
    Ui::AddDockerServiceDlg *ui;

    QPoint m_last;
    bool m_mousePress;
    std::string m_addr_port;

    WalletModel *m_walletModel;

private:
    bool createDockerService();
    void showStep(int);

    bool doStep1();
    bool doStep3();
    bool doStep4();
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private Q_SLOTS:
    bool slot_okbutton();
    void slot_openPubKeyFile();
    bool doStep2();
    void slot_nextStep();
    void slot_imageCurrentChanged(int);
    void slot_close();
    void slot_timeOut();

};

#endif // ADDDOCKERSERVICEDLG_H
