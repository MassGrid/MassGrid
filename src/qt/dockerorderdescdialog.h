#ifndef DOCKERORDERDESCDIALOG_H
#define DOCKERORDERDESCDIALOG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class DockerOrderDescDialog;
}

class CWalletTx;
class WalletModel;
class DockerOrderDescDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DockerOrderDescDialog(WalletModel *model,QWidget *parent = 0);
    ~DockerOrderDescDialog();

    void setwalletTx(CWalletTx& wtx);

private:
    Ui::DockerOrderDescDialog *ui;
    QPoint m_last;
    bool m_mousePress;

    WalletModel *m_walletModel;

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // DOCKERORDERDESCDIALOG_H
