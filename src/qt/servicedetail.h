#ifndef SERVICEDETAIL_H
#define SERVICEDETAIL_H

#include <QDialog>
#include <QMouseEvent>

class ServiceInfo;
class Task;

class WalletModel;

namespace Ui {
class ServiceDetail;
}

class ServiceDetail : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceDetail(QWidget *parent = 0);
    ~ServiceDetail();

    void setService(ServiceInfo& service);
    void setModel(WalletModel* model);

private:
    Ui::ServiceDetail *ui;

    WalletModel* m_walletModel;

    QPoint m_last;
    bool m_mousePress;

    void updateTaskDetail(std::vector<Task> &mapDockerTasklists,int& taskStatus);
    void updateServiceDetail(ServiceInfo& service);


protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // SERVICEDETAIL_H
