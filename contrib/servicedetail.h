#ifndef SERVICEDETAIL_H
#define SERVICEDETAIL_H

#include <QDialog>

namespace Ui {
class ServiceDetail;
}

class ServiceDetail : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceDetail(QWidget *parent = 0);
    ~ServiceDetail();

private:
    Ui::ServiceDetail *ui;
};

#endif // SERVICEDETAIL_H
