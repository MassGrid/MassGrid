#ifndef DOCKERORDERDESCDIALOG_H
#define DOCKERORDERDESCDIALOG_H

#include <QDialog>

namespace Ui {
class DockerOrderDescDialog;
}

class DockerOrderDescDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DockerOrderDescDialog(QWidget *parent = 0);
    ~DockerOrderDescDialog();

private:
    Ui::DockerOrderDescDialog *ui;
};

#endif // DOCKERORDERDESCDIALOG_H
