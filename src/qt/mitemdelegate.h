#ifndef MITEMDELEGATE_H
#define MITEMDELEGATE_H


//class MItemDelegate : public QStyledItemDelegate
//{
//public:
//    MItemDelegate();
//};


#include <QObject>
#include <qstyleditemdelegate.h>

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


#endif // MITEMDELEGATE_H
