#ifndef DEFINECALENDAR_H
#define DEFINECALENDAR_

#include <QWidget>
#include <QKeyEvent>

#include <QDateTimeEdit>
#include <QDateTime>
#include <QDateEdit>
#include <QCalendarWidget>

#include <QObject>
#include <qstyleditemdelegate.h>

class QPushButton;
class QComboBox;

class DefineCalendar : public QCalendarWidget
{
    Q_OBJECT

public:
    DefineCalendar(QWidget *parent);
    ~DefineCalendar();

Q_SIGNALS:
    void setFinished(const QDateTime &dateTime);

public Q_SLOTS:
    void UpdateYear();
    void UpdatePage();
    void SetToday();
    void ClearTime();

protected Q_SLOTS:
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

protected Q_SLOTS:
    void getDateTime(const QDateTime &dateTime);

private:
    DefineCalendar *m_DefCalendar;

};


class MItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    MItemDelegate(QObject *parent=0);
    ~MItemDelegate();

private:

    void paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


#endif // DEFINECALENDAR_H