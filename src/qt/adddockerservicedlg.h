#ifndef ADDDOCKERSERVICEDLG_H
#define ADDDOCKERSERVICEDLG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class AddDockerServiceDlg;
}

class AddDockerServiceDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AddDockerServiceDlg(QWidget *parent = nullptr);
    ~AddDockerServiceDlg();

    void setaddr_port(const std::string& addr_poprt);

private:
    Ui::AddDockerServiceDlg *ui;

    QPoint m_last;
    bool m_mousePress;
    std::string m_addr_port;

private:
    bool createDocketService();
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private Q_SLOTS:
    void slot_okbutton();

};

#endif // ADDDOCKERSERVICEDLG_H
