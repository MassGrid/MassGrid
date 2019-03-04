#ifndef SERVICEDETAIL_H
#define SERVICEDETAIL_H

#include <QDialog>
#include <QMouseEvent>

class Service;
class Task;

namespace Ui {
class ServiceDetail;
}

class ServiceDetail : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceDetail(QWidget *parent = 0);
    ~ServiceDetail();

    void setService(Service& service);

private:
    Ui::ServiceDetail *ui;

    QPoint m_last;
    bool m_mousePress;

    void updateTaskDetail(std::map<std::string,Task> &mapDockerTasklists,int& taskStatus);
    void updateServiceDetail(Service& service);


protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // SERVICEDETAIL_H
