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

    void setwalletTx(const std::string& txid,CWalletTx& wtx);

private:
    Ui::DockerOrderDescDialog *ui;
    QPoint m_last;
    bool m_mousePress;
    std::string m_createOutPoint;

    WalletModel *m_walletModel;

    void loadRerentView();
    void resetTableWidgetTitle();
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *event);

private Q_SLOTS:
    void slot_curTabPageChanged(int index);
};

#endif // DOCKERORDERDESCDIALOG_H
