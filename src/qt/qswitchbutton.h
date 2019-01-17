#ifndef QSWITCHBUTTON_H
#define QSWITCHBUTTON_H

#include <QLabel>
#include <QObject>

class QSwitchButton : public QLabel
{
    Q_OBJECT

Q_SIGNALS:

    void clicked();
    void clicked(bool is_selected);

public:
    QSwitchButton(QWidget *parent = nullptr);
    ~QSwitchButton();

    typedef enum _buttonStyle{
        Rectage,    
        Ellipse,    
    }ButtonStyle;

    void SetButtonStyle(ButtonStyle button_style);

    void SetSize(int width, int height);
    void SetSize(const QSize& size);

    void SetBackgroundColor(const QColor& selected_color, const QColor& not_selected_color);

    void SetSliderColor(const QColor& selected_color, const QColor& not_selected_color);

    void SetRound(int round);

    void EnableAntiAliasing(bool enable);
    bool IsAntiAliasing();

    void SetSelected(bool is_selected);
    bool IsSelected();

    void SetEnabel(bool is_enable);
    bool IsEnable();

protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private:
    void DrawBackRect(QPainter* painter, const QRectF& rect);

    void DrawSliderRect(QPainter* painter, const QRectF& rect);

private:
    bool isEnable;  

    bool isSelected;    
    bool isAntiAliasing;    

    int buttonWidth;
    int buttonHeight;

    QColor backgroundColorSelected;
    QColor backgroundColorNotSelected;

    QColor sliderColorSelected;
    QColor sliderColorNotSelected;

    int rectRound;

    ButtonStyle buttonStyle;
};

#endif // QSWITCHBUTTON_H
