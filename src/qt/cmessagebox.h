#ifndef CMESSAGEBOX_H
#define CMESSAGEBOX_H

#include <QObject>
#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class CMessageBox;
}

class CMessageBox : public QDialog
{
    Q_OBJECT
public:
    explicit CMessageBox(QWidget *parent,QString title,QString text);
    ~CMessageBox();

    enum StandardButton {
        // keep this in sync with QDialogButtonBox::StandardButton and QPlatformDialogHelper::StandardButton
        Ok = 1,
        Cancel,
        Ok_Cancel,
        NoButton
    };

    void setmode(StandardButton);

    static StandardButton information(QWidget *parent, const QString &title,
                                       const QString &text, StandardButton buttons = CMessageBox::Ok,
                                       StandardButton defaultButton = CMessageBox::NoButton);
    static StandardButton critical(QWidget *parent, const QString &title,
         const QString &text, StandardButton buttons = CMessageBox::Ok,
         StandardButton defaultButton = CMessageBox::NoButton);

    static StandardButton question(QWidget *parent, const QString &title,
         const QString &text, StandardButton buttons = CMessageBox::Ok,
         StandardButton defaultButton = CMessageBox::NoButton);

    static StandardButton warning(QWidget *parent, const QString &title,
         const QString &text, StandardButton buttons = CMessageBox::Ok,
         StandardButton defaultButton = CMessageBox::NoButton); 

private:
    Ui::CMessageBox *ui;

    QPoint m_last;
    bool m_mousePress;

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private Q_SLOTS:
    void slot_close();
    void slot_ok();

};

#endif // CMESSAGEBOX_H
