#ifndef ORDERDETAIL_H
#define ORDERDETAIL_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class OrderDetail;
}

class OrderDetail : public QDialog
{
    Q_OBJECT

public:
    explicit OrderDetail(QWidget *parent = 0);
    ~OrderDetail();

private:
    Ui::OrderDetail *ui;

    QPoint m_last;
    bool m_mousePress;

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // ORDERDETAIL_H
