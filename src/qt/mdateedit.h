#ifndef MDATEEDIT_H
#define MDATEEDIT_H

#include <QDateTimeEdit>
#include <QDateTime>
#include <QDateEdit>
#include <QCalendarWidget>

//class DefineCalendar;



QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QPushButton;
class QTimeEdit;
class QDateTime;
QT_END_NAMESPACE

class DefineCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    DefineCalendar(QWidget *parent);
    ~DefineCalendar();

signals:
    void setFinished(const QDateTime &dateTime);

public slots:
    void UpdateYear();
    void UpdatePage();
    void SetToday();
    void ClearTime();

protected slots:
    void BtnSlots();
    void ComboBoxSlots(int index);
    void CurPageChange(int year, int month);

protected:
    void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const;

private:
    void InitWidgets();
    QWidget *widget_top;
    QPushButton *pushBtn_YL;
    QComboBox *comboBox_Year;
    QPushButton *pushBtn_YR;
    QPushButton *pushBtn_ML;
    QComboBox *comboBox_Month;
    QPushButton *pushBtn_MR;
};


class MDateEdit : public QDateEdit
{
    Q_OBJECT

public:
    MDateEdit(QWidget *parent);
    ~MDateEdit();
    void setMyStytle();

protected slots:
    void getDateTime(const QDateTime &dateTime);

private:
    DefineCalendar *m_DefCalendar;

};

#endif // MDATEEDIT_H
