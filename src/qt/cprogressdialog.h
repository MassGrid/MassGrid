#ifndef CPROGRESSDIALOG_H
#define CPROGRESSDIALOG_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class CProgressDialog;
}

class CProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CProgressDialog(const QString &labelText, int minimum, int maximum, QWidget *parent = Q_NULLPTR);
    ~CProgressDialog();

    void setValue(int value);

private:
    Ui::CProgressDialog *ui;

    QPoint m_last;
    bool m_mousePress;

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};

#endif // CPROGRESSDIALOG_H
