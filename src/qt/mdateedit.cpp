#include "mdateedit.h"
#include <QtWidgets/QtWidgets>

MDateEdit::MDateEdit(QWidget *parent)
    : QDateEdit(parent)
{
    this->setCalendarPopup(true);
    m_DefCalendar = new DefineCalendar(this);
    m_DefCalendar->setLocale(QLocale(QLocale::English));

    setCalendarWidget(m_DefCalendar);

    m_DefCalendar->disconnect(SIGNAL(clicked(QDate)));
    connect(m_DefCalendar, SIGNAL(setFinished(QDateTime)), this, SLOT(getDateTime(QDateTime)));

    setMyStytle();

}

MDateEdit::~MDateEdit()
{

}

void MDateEdit::getDateTime( const QDateTime &dateTime )
{
    this->setDateTime(dateTime);
}

void MDateEdit::setMyStytle()
{
    QString strTemp;
    //QWidget
    strTemp.append("QWidget{font:normal 10pt Microsoft YaHei;}");
    strTemp.append("QWidget#CalTopWidget{background-color:rgb(172,99,43);}");
    strTemp.append("QWidget#CalBottomWidget{background-color:white;}");
    //QLabel
    strTemp.append("QLabel#CalLabel{border:1px solid lightgray; color:rgb(172,99,43);}");

    //QPushButton
    strTemp.append("QPushButton#CalPushBtnT1{border:0px;}");
    strTemp.append("QPushButton#CalPushBtnT1:hover,QPushButton#CalPushBtnT1:pressed{background-color:rgb(239,169,4);}");
                   strTemp.append("QPushButton#CalPushBtnT2{border:1px solid lightgray; color:rgb(172,99,43);}");
            strTemp.append("QPushButton#CalPushBtnT2:hover,QPushButton#CalPushBtnT2:pressed{background-color:rgb(231, 231, 231);}");
   //QComboBox
   strTemp.append("QComboBox#CalComboBox{border:0px; background-color:rgb(172,99,43); \
                    color:white; height:24px; width:40px;}\
                    QComboBox#CalComboBox::down-arrow{\
                    image:url(://icon/ad.png);}\
                    QComboBox#CalComboBox::drop-down{width:14px; border:0px;}\
                    QComboBox#CalComboBox QAbstractItemView {\
                    color:rgb(172,99,43);\
                    border: 1px solid rgb(172,99,43);\
                        background-color:white;\
                        selection-color:white;\
                        selection-background-color: rgb(239,169,4);}");
    //QTimeEdit
    strTemp.append("QTimeEdit#CalTimeEdit{ border:1px solid lightgray; color:rgb(172,99,43);}");
    strTemp.append("QTimeEdit#CalTimeEdit:!enabled{ background:rgb(65, 65, 65); color:rgb(90, 90, 90); border:0px; }");
    strTemp.append("QTimeEdit#CalTimeEdit::up-button{  background:rgb(172,99,43);width: 16px;  border-width: 1px;}");
    strTemp.append("QTimeEdit#CalTimeEdit::up-button:hover{ background:rgb(239,169,4); }");
    strTemp.append("QTimeEdit#CalTimeEdit::up-button:!enabled{ background:rgb(65, 65, 65); }");
    strTemp.append("QTimeEdit#CalTimeEdit::up-arrow{  image:url(://icon/au.png);}");

    strTemp.append("QTimeEdit#CalTimeEdit::down-button{  background:rgb(172,99,43); width: 16px;  border-width: 1px;}");
    strTemp.append("QTimeEdit#CalTimeEdit::down-button:hover{ background:rgb(239,169,4); }");
    strTemp.append("QTimeEdit#CalTimeEdit::down-button:!enabled{ background:rgb(65, 65, 65); }");
    strTemp.append("QTimeEdit#CalTimeEdit::down-arrow{  image:url(://icon/ad.png);}");

    //QDateEdit
    strTemp.append("QDateEdit{border:1px solid gray; height:24px; }");
    strTemp.append("QDateEdit::down-arrow{image:url(://icon/calendar.png);}");
    strTemp.append("QDateEdit::drop-down{width:30px; border:0px solid red;\
                   subcontrol-origin: padding;subcontrol-position: top right;}");

    //QScrollBar
    strTemp.append("QScrollBar:vertical{background-color:white; width: 10px;}\
                   QScrollBar::handle:vertical{background-color:rgb(172,99,43); border:1px solid white;border-radius:2px; min-height:8px}\
                   QScrollBar::handle:vertical:hover{background-color:rgb(239,169,4);}\
                   QScrollBar::sub-line{background-color:white;}\
                   QScrollBar::add-line{background-color:white;}");

                   this->setStyleSheet(strTemp);
}


DefineCalendar::DefineCalendar(QWidget *parent)
    : QCalendarWidget(parent)
{
    setMinimumDate(QDate(2000,1,1));
    setMaximumDate(QDate(2100,1,1));
    InitWidgets();
    setMinimumHeight(240);
    setNavigationBarVisible(false);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
    connect(this, SIGNAL(currentPageChanged(int,int)), this, SLOT(CurPageChange(int,int)));
    UpdateYear();

}

DefineCalendar::~DefineCalendar()
{

}

void DefineCalendar::paintCell( QPainter *painter, const QRect &rect, const QDate &date ) const
{
    if (date == this->selectedDate())
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(172,99,43));
        painter->drawRect(rect);
        painter->setPen(Qt::white);
        painter->drawText(rect,Qt::AlignCenter,QString::number(date.day()));
        painter->restore();
    }
    else
    {
        QCalendarWidget::paintCell(painter,rect,date);
    }

}

void DefineCalendar::InitWidgets()
{
    //top
    widget_top = new QWidget(this);
    comboBox_Year = new QComboBox(this);
    comboBox_Month = new QComboBox(this);
    pushBtn_YL = new QPushButton(this);
    pushBtn_YR = new QPushButton(this);
    pushBtn_ML = new QPushButton(this);
    pushBtn_MR = new QPushButton(this);

    comboBox_Year->setView(new QListView());
    comboBox_Month->setView(new QListView());

    QStringList monthList;
    monthList<<tr("Jan")<<tr("Feb")<<tr("Mar")<<tr("Apr")<<tr("May")<<tr("Jun")
            <<tr("Jul")<<tr("Aug")<<tr("Sep")<<tr("Oct")<<tr("Nov")<<tr("Dec");
    comboBox_Month->addItems(monthList);


    int nO = 24;
    int nI = 20;
    pushBtn_YL->setFixedSize(nO,nO);
    pushBtn_YL->setIconSize(QSize(nI,nI));
    pushBtn_YL->setIcon(QPixmap("://icon/al.png"));

    pushBtn_YR->setFixedSize(nO,nO);
    pushBtn_YR->setIconSize(QSize(nI,nI));
    pushBtn_YR->setIcon(QPixmap("://icon/ar.png"));

    pushBtn_ML->setFixedSize(nO,nO);
    pushBtn_ML->setIconSize(QSize(nI,nI));
    pushBtn_ML->setIcon(QPixmap("://icon/al.png"));

    pushBtn_MR->setFixedSize(nO,nO);
    pushBtn_MR->setIconSize(QSize(nI,nI));
    pushBtn_MR->setIcon(QPixmap("://icon/ar.png"));

    QHBoxLayout *HTopLayout = new QHBoxLayout;
    HTopLayout->setContentsMargins(4,4,4,4);
    HTopLayout->setSpacing(0);
    widget_top->setLayout(HTopLayout);
    HTopLayout->addWidget(pushBtn_YL);
    HTopLayout->addWidget(comboBox_Year);
    HTopLayout->addWidget(pushBtn_YR);
    HTopLayout->addStretch(1);
    HTopLayout->addWidget(pushBtn_ML);
    HTopLayout->addWidget(comboBox_Month);
    HTopLayout->addWidget(pushBtn_MR);

    QVBoxLayout *VMainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    VMainLayout->insertWidget(0,widget_top);

    widget_top->setObjectName("CalTopWidget");
    comboBox_Year->setObjectName("CalComboBox");
    comboBox_Month->setObjectName("CalComboBox");
    pushBtn_YL->setObjectName("CalPushBtnT1");
    pushBtn_YR->setObjectName("CalPushBtnT1");
    pushBtn_ML->setObjectName("CalPushBtnT1");
    pushBtn_MR->setObjectName("CalPushBtnT1");


    connect(pushBtn_YL, SIGNAL(clicked()), this, SLOT(BtnSlots()));
    connect(pushBtn_YR, SIGNAL(clicked()), this, SLOT(BtnSlots()));
    connect(pushBtn_ML, SIGNAL(clicked()), this, SLOT(BtnSlots()));
    connect(pushBtn_MR, SIGNAL(clicked()), this, SLOT(BtnSlots()));

    connect(comboBox_Year, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboBoxSlots(int)));
    connect(comboBox_Month, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboBoxSlots(int)));
}

void DefineCalendar::UpdateYear()
{
    comboBox_Year->clear();
    QDate d1 = this->minimumDate();
    QDate d2 = this->maximumDate();
    for (int i = d1.year(); i<= d2.year(); i++)
    {
        comboBox_Year->addItem(tr("%1").arg(i));
    }
}

void DefineCalendar::SetToday()
{
    QDate curDate = QDate::currentDate();
    int year = curDate.year();
    int month = curDate.month();
    this->setSelectedDate(curDate);
    comboBox_Year->setCurrentText(QString::number(year));
    comboBox_Month->setCurrentIndex(month-1);
}

void DefineCalendar::ClearTime()
{

}

void DefineCalendar::BtnSlots()
{
    QPushButton *pBtn = qobject_cast<QPushButton *>(sender());

    if (pBtn == pushBtn_YL)
    {
        int curInt = comboBox_Year->currentIndex()-1;
        if (curInt<=0)
        {
            curInt = 0;
        }
        comboBox_Year->setCurrentIndex(curInt);
    }
    else if (pBtn == pushBtn_YR)
    {
        int curInt = comboBox_Year->currentIndex()+1;
        if (curInt > comboBox_Year->count()-1)
        {
            curInt = comboBox_Year->count()-1;
        }
        comboBox_Year->setCurrentIndex(curInt);
    }
    else if (pBtn == pushBtn_ML)
    {
        int curInt = comboBox_Month->currentIndex()-1;
        if (curInt<=0)
        {
            curInt = 0;
        }
        comboBox_Month->setCurrentIndex(curInt);
    }
    else if (pBtn == pushBtn_MR)
    {
        int curInt = comboBox_Month->currentIndex()+1;
        if (curInt > comboBox_Month->count()-1)
        {
            curInt = comboBox_Month->count()-1;
        }
        comboBox_Month->setCurrentIndex(curInt);
    }
    UpdatePage();
}

void DefineCalendar::ComboBoxSlots( int index )
{
    UpdatePage();
}

void DefineCalendar::UpdatePage()
{
    int nYear = comboBox_Year->currentText().toInt();
    int nMonth = comboBox_Month->currentIndex()+1;
    this->setCurrentPage(nYear,nMonth);
}

void DefineCalendar::CurPageChange( int year, int month )
{
    comboBox_Year->setCurrentText(QString::number(year));
    comboBox_Month->setCurrentIndex(month-1);
}
