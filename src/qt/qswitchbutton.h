#ifndef QSWITCHBUTTON_H
#define QSWITCHBUTTON_H

#include <QLabel>
#include <QObject>

class QSwitchButton : public QLabel
{
    Q_OBJECT

Q_SIGNALS:
    /*
     * @brief       button点击信号
     * @param[in]   is_selected 当前是否选中
     */
    void clicked();
    void clicked(bool is_selected);

public:
    QSwitchButton(QWidget *parent = nullptr);
    ~QSwitchButton();

    /*
     * @brief   按钮样式
     */
    typedef enum _buttonStyle{
        Rectage,    //矩形样式
        Ellipse,    //椭圆样式
    }ButtonStyle;

    /*
     * @brief   设置按钮样式
     */
    void SetButtonStyle(ButtonStyle button_style);

    /*
     * @brief       设置button的大小
     */
    void SetSize(int width, int height);
    void SetSize(const QSize& size);

    /*
     * @brief       设置背景颜色
     * @param[in]   selected_color      选中的颜色
     * @param[in]   not_selected_color  未选中的颜色
     */
    void SetBackgroundColor(const QColor& selected_color, const QColor& not_selected_color);

    /*
     * @brief       这是滑块颜色
     * @param[in]   selected_color      选中的颜色
     * @param[in]   not_selected_color  未选中的颜色
     */
    void SetSliderColor(const QColor& selected_color, const QColor& not_selected_color);

    /*
     * @brief       设置圆角弧度
     * @param[in]   round   设置的弧度
     */
    void SetRound(int round);

    /*
     * @brief       是否开启抗锯齿
     */
    void EnableAntiAliasing(bool enable);
    bool IsAntiAliasing();

    /*
     * @brief   当前是否是选中状态
     */
    void SetSelected(bool is_selected);
    bool IsSelected();

    /*
     * @brief   是否启用
     */
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
    bool isEnable;  //是否启用

    bool isSelected;    //当前是否为选中状态
    bool isAntiAliasing;    //是否开启抗锯齿

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
