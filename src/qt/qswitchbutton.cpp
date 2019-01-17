#include "qswitchbutton.h"
#include <QPainter>
#include <QRectF>

QSwitchButton::QSwitchButton(QWidget *parent)
    : QLabel(parent), isSelected(true), buttonWidth(100), buttonHeight(40)
    , backgroundColorSelected("#E7E3E7"), backgroundColorNotSelected("#E7E3E7")
    , sliderColorSelected("#EFA904"), sliderColorNotSelected("#7B797B")
    , rectRound(5), isAntiAliasing(true), buttonStyle(Rectage), isEnable(true)
{
    resize(buttonWidth, buttonHeight);
    setMouseTracking(true);
}

QSwitchButton::~QSwitchButton()
{

}

void QSwitchButton::SetButtonStyle(ButtonStyle button_style)
{
    buttonStyle = button_style;
    repaint();
}

void QSwitchButton::SetSize(int width, int height)
{
    buttonWidth = width;
    buttonHeight = height;

    resize(buttonWidth, buttonHeight);

    repaint();
}

void QSwitchButton::SetSize(const QSize& size)
{
    buttonWidth = size.width();
    buttonHeight = size.height();

    resize(buttonWidth, buttonHeight);

    repaint();
}

void QSwitchButton::SetBackgroundColor(const QColor& selected_color, const QColor& not_selected_color)
{
    backgroundColorSelected = selected_color;
    backgroundColorNotSelected = not_selected_color;

    repaint();
}

void QSwitchButton::SetSliderColor(const QColor& selected_color, const QColor& not_selected_color)
{
    sliderColorSelected = selected_color;
    sliderColorNotSelected = not_selected_color;

    repaint();
}

void QSwitchButton::SetRound(int round)
{
    rectRound = round;
    repaint();
}

void QSwitchButton::EnableAntiAliasing(bool enable)
{
    isAntiAliasing = enable;
    repaint();
}

bool QSwitchButton::IsAntiAliasing()
{
    return isAntiAliasing;
}

void QSwitchButton::SetSelected(bool is_selected)
{
    isSelected = is_selected;

    repaint();
}

bool QSwitchButton::IsSelected()
{
    return isSelected;
}

void QSwitchButton::SetEnabel(bool is_enable)
{
    isEnable = is_enable;

    repaint();
}

bool QSwitchButton::IsEnable()
{
    return isEnable;
}

void QSwitchButton::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (isAntiAliasing)
    {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
    }

    if (!isEnable)  
    {
        painter.setPen(QPen(QColor("#F4F4F4")));
        painter.setBrush(QBrush(QColor("#F4F4F4")));
        DrawBackRect(&painter, QRectF(0, 0, buttonWidth, buttonHeight));

        painter.setPen(QPen(QColor("#ADB2B5")));
        painter.setBrush(QBrush(QColor("#ADB2B5")));
        DrawSliderRect(&painter, QRectF(buttonWidth*2/3-2, 2.5, buttonWidth/3, buttonHeight-5));
    }
    else if (isSelected)
    {
        painter.setPen(QPen(backgroundColorSelected));
        painter.setBrush(QBrush(backgroundColorSelected));
        DrawBackRect(&painter, QRectF(0, 0, buttonWidth, buttonHeight));

        painter.setPen(QPen(sliderColorSelected));
        painter.setBrush(QBrush(sliderColorSelected));
        DrawSliderRect(&painter, QRectF(buttonWidth*2/3-2, 2.5, buttonWidth/3, buttonHeight-5));
    }
    else
    {
        painter.setPen(QPen(backgroundColorNotSelected));
        painter.setBrush(QBrush(backgroundColorNotSelected));
        DrawBackRect(&painter,QRectF(0, 0, buttonWidth, buttonHeight));

        painter.setPen(QPen(sliderColorNotSelected));
        painter.setBrush(QBrush(sliderColorNotSelected));
        DrawSliderRect(&painter,QRectF(2, 2.5, buttonWidth/3, buttonHeight-5));
    }

    return QLabel::paintEvent(event);
}

void QSwitchButton::mousePressEvent(QMouseEvent *event)
{
    if (isEnable)
    {
        isSelected = !isSelected;
        repaint();

        Q_EMIT clicked();
        Q_EMIT clicked(isSelected);
    }

    return QLabel::mousePressEvent(event);
}

void QSwitchButton::mouseMoveEvent(QMouseEvent *event)
{
    setCursor(Qt::PointingHandCursor);

    return QLabel::mouseMoveEvent(event);
}

void QSwitchButton::DrawBackRect(QPainter* painter, const QRectF& rect)
{
    QPen pen;
    pen = painter->pen();
    pen.setColor(Qt::black);

    switch (buttonStyle)
    {
    case Rectage:
        painter->drawRoundRect(rect, rectRound, rectRound);
        painter->setPen(pen);
        painter->drawText(rect,Qt::AlignCenter, tr("Edge"));
        break;
    case Ellipse:
        painter->drawEllipse(0, 0, buttonHeight, buttonHeight);
        painter->drawEllipse(buttonWidth - buttonHeight, 0, buttonHeight, buttonHeight);
        painter->drawRect(buttonHeight/2, 0, buttonWidth-buttonHeight, buttonHeight);
        painter->setPen(pen);
        painter->drawText(rect,Qt::AlignCenter,tr("Edge"));
        break;
    default:
        break;
    }
}

void QSwitchButton::DrawSliderRect(QPainter* painter, const QRectF& rect)
{
    QPen pen;
    switch (buttonStyle)
    {
    case Rectage:
        painter->drawRoundRect(rect, rectRound, rectRound);
        pen = painter->pen();
        pen.setColor(Qt::black);
        painter->setPen(pen);
        painter->drawText(rect,Qt::AlignCenter,IsSelected()?tr("OFF"):tr("ON"));
        break;
    case Ellipse:
        painter->drawEllipse(rect);
        pen = painter->pen();
        pen.setColor(Qt::black);
        painter->setPen(pen);
        painter->drawText(rect,Qt::AlignCenter,IsSelected()?tr("OFF"):tr("ON"));
        break;
    default:
        break;
    }
}
