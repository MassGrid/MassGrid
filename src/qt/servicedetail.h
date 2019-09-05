#ifndef SERVICEDETAIL_H
#define SERVICEDETAIL_H

#include <QDialog>
#include <QMouseEvent>
#include <QResizeEvent>
// #include "primitives/transaction.h"

class ServiceInfo;
class Task;

class WalletModel;
// class COutPoint;

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
    std::string m_createOutPoint;

    QPoint m_last;
    bool m_mousePress;

    void updateTaskDetail(std::vector<Task> &mapDockerTasklists,int& taskStatus);
    void updateServiceDetail(ServiceInfo& service);
    void loadRerentView();
    void resetTableWidgetTitle();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *event);
    
private Q_SLOTS:
    void slot_curTabPageChanged(int);
};

#endif // SERVICEDETAIL_H
