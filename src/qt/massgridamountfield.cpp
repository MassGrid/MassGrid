// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "massgridamountfield.h"

#include "massgridunits.h"
#include "guiconstants.h"
#include "qvaluecombobox.h"

#include <QApplication>
#include <QAbstractSpinBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSizePolicy>
#include <QStyleFactory>

/** QSpinBox that uses fixed-point numbers internally and uses our own
 * formatting/parsing functions.
 */
class AmountSpinBox: public QAbstractSpinBox
{
    Q_OBJECT

public:
    explicit AmountSpinBox(QWidget *parent):
        QAbstractSpinBox(parent),
        currentUnit(MassGridUnits::MGD),
        singleStep(100000) // satoshis
    {
        setAlignment(Qt::AlignRight);

        connect(lineEdit(), SIGNAL(textEdited(QString)), this, SIGNAL(valueChanged()));
    }

    QValidator::State validate(QString &text, int &pos) const
    {
        if(text.isEmpty())
            return QValidator::Intermediate;
        bool valid = false;
        parse(text, &valid);
        /* Make sure we return Intermediate so that fixup() is called on defocus */
        return valid ? QValidator::Intermediate : QValidator::Invalid;
    }

    void fixup(QString &input) const
    {
        bool valid = false;
        CAmount val = parse(input, &valid);
        if(valid)
        {
            input = MassGridUnits::format(currentUnit, val, false, MassGridUnits::separatorAlways);
            lineEdit()->setText(input);
        }
    }

    CAmount value(bool *valid_out=0) const
    {
        return parse(text(), valid_out);
    }

    void setValue(const CAmount& value)
    {
        lineEdit()->setText(MassGridUnits::format(currentUnit, value, false, MassGridUnits::separatorAlways));
        Q_EMIT valueChanged();
    }

    void stepBy(int steps)
    {
        bool valid = false;
        CAmount val = value(&valid);
        val = val + steps * singleStep;
        val = qMin(qMax(val, CAmount(0)), MassGridUnits::maxMoney());
        setValue(val);
    }

    void setDisplayUnit(int unit)
    {
        bool valid = false;
        CAmount val = value(&valid);

        currentUnit = unit;

        if(valid)
            setValue(val);
        else
            clear();
    }

    void setSingleStep(const CAmount& step)
    {
        singleStep = step;
    }

    QSize minimumSizeHint() const
    {
        if(cachedMinimumSizeHint.isEmpty())
        {
            ensurePolished();

            const QFontMetrics fm(fontMetrics());
            int h = lineEdit()->minimumSizeHint().height();
            int w = fm.width(MassGridUnits::format(MassGridUnits::MGD, MassGridUnits::maxMoney(), false, MassGridUnits::separatorAlways));
            w += 2; // cursor blinking space

            QStyleOptionSpinBox opt;
            initStyleOption(&opt);
            QSize hint(w, h);
            QSize extra(35, 6);
            opt.rect.setSize(hint + extra);
            extra += hint - style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                                    QStyle::SC_SpinBoxEditField, this).size();
            // get closer to final result by repeating the calculation
            opt.rect.setSize(hint + extra);
            extra += hint - style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                                    QStyle::SC_SpinBoxEditField, this).size();
            hint += extra;
            hint.setHeight(h);

            opt.rect = rect();

            cachedMinimumSizeHint = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                                    .expandedTo(QApplication::globalStrut());
        }
        return cachedMinimumSizeHint;
    }

private:
    int currentUnit;
    CAmount singleStep;
    mutable QSize cachedMinimumSizeHint;

    /**
     * Parse a string into a number of base monetary units and
     * return validity.
     * @note Must return 0 if !valid.
     */
    CAmount parse(const QString &text, bool *valid_out=0) const
    {
        CAmount val = 0;
        bool valid = MassGridUnits::parse(currentUnit, text, &val);
        if(valid)
        {
            if(val < 0 || val > MassGridUnits::maxMoney())
                valid = false;
        }
        if(valid_out)
            *valid_out = valid;
        return valid ? val : 0;
    }

protected:
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Comma)
            {
                // Translate a comma into a period
                QKeyEvent periodKeyEvent(event->type(), Qt::Key_Period, keyEvent->modifiers(), ".", keyEvent->isAutoRepeat(), keyEvent->count());
                return QAbstractSpinBox::event(&periodKeyEvent);
            }
        }
        return QAbstractSpinBox::event(event);
    }

    StepEnabled stepEnabled() const
    {
        if (isReadOnly()) // Disable steps when AmountSpinBox is read-only
            return StepNone;
        if (text().isEmpty()) // Allow step-up with empty field
            return StepUpEnabled;

        StepEnabled rv = 0;
        bool valid = false;
        CAmount val = value(&valid);
        if(valid)
        {
            if(val > 0)
                rv |= StepDownEnabled;
            if(val < MassGridUnits::maxMoney())
                rv |= StepUpEnabled;
        }
        return rv;
    }

Q_SIGNALS:
    void valueChanged();
};

#include "massgridamountfield.moc"

MassGridAmountField::MassGridAmountField(QWidget *parent) :
    QWidget(parent),
    amount(0)
{
    amount = new AmountSpinBox(this);
    amount->setLocale(QLocale::c());
    amount->installEventFilter(this);
    amount->setMaximumWidth(170);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(amount);
    unit = new QValueComboBox(this);
    unit->setModel(new MassGridUnits(this));
    QSizePolicy amountSizePolicy = amount->sizePolicy();
    amountSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
    amount->setSizePolicy(amountSizePolicy);

    QSizePolicy unitSizePolicy = unit->sizePolicy();
    unitSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
    unit->setSizePolicy(unitSizePolicy);
    layout->addWidget(unit);
    layout->addStretch(1);
    layout->setContentsMargins(0,0,0,0);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(valueChanged()), this, SIGNAL(valueChanged()));
    connect(unit, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));

    // Set default based on configuration
    unitChanged(unit->currentIndex());
	
    // amount->setStyleSheet("AmountSpinBox{\n border:hidden; color:rgb(255,255,255); \n}\nAmountSpinBox::up-button\n{\n height: 0px;\nwidth:0px;\n}\nAmountSpinBox::down-button{\nheight: 0px;\nwidth:0px;\n}");
    amount->setStyleSheet("AmountSpinBox{\n border:hidden; color:rgba(255,255,255,0); \n}\nAmountSpinBox::up-button\n{\n height: 0px;\nwidth:0px;\n}\nAmountSpinBox::down-button{\nheight: 0px;\nwidth:0px;\n}");

    // amount->setStyleSheet("AmountSpinBox{\n height:32px;\n min-width:250px;\n border:hidden; color:rgb(0,0,0); \n}\nAmountSpinBox::up-button\n{\n height: 0px;\nwidth:0px;\n}\nAmountSpinBox::down-button{\nheight: 0px;\nwidth:0px;\n}");
// #ifdef Q_OS_MAC
//     unit->setStyle(QStyleFactory::create("Windows"));
//     unit->setStyleSheet("QComboBox{\n    background-color:rgb(172,99,43); \n    color:white;\n    height:30px;\n    width:120px;\n    border:0px solid rgb(174,103,46);\n    background-repeat: no-repeat;\n    background-position: center left;\n    background-color: rgb(255, 255, 255);\n    color: rgb(0, 0, 0);\n    selection-color: black;\n    selection-background-color: darkgray;\n}\nQComboBox::drop-down\n{\n    width: 30px;\n    height:30px;\n    image: url(:/res/pic/xjt.png);\n}\n\nQComboBox QAbstractItemView\n{\n    width:140px;\n  outline: 0px;\n  color: rgb(255, 255, 255);\n    selection-color: rgb(255, 255, 255);\n    selection-background-color: rgb(239, 169, 4);\n    background-color: rgb(198, 125, 26);\n}\nQComboBox QAbstractItemView::item\n{\n    height: 40px;\n   background-color: rgb(198, 125, 26);\n    border:hidden;\n    color: rgb(255, 255, 255);\n}");
// #else
    unit->setStyleSheet("QValueComboBox\n{ color:rgba(0,0,0,0); border:0px solid rgb(174,103,46);\nfont-size: 12pt;\nbackground-repeat: no-repeat;\nbackground-position: center left;\nbackground-color: rgb(255, 255, 255);\ncolor: rgb(0, 0, 0);\nselection-color: black;\nselection-background-color: darkgray;\n}\n\nQComboBox::drop-down \n{\nwidth: 30px; \nheight:30px;\nimage: url(:/res/pic/xjt.png);\n}\nQComboBox QAbstractItemView\n{\nheight:100px;\nborder: 0px; outline: 0px;  \ncolor: rgb(255, 255, 255);\nselection-color: rgb(255, 255, 255);\nselection-background-color: rgb(239, 169, 4);\nbackground-color: rgb(198, 125, 26);\n}\nQComboBox QAbstractItemView::item\n{\nheight: 20px;\nbackground-color: rgb(198, 125, 26);\nborder:hidden;\ncolor: rgb(255, 255, 255);\n}\n\n");
// #endif
    unit->setMaximumWidth(120);
    unit->setMaximumHeight(28);

    amount->setMaximumHeight(28);

    setMinimumWidth(400);
}

void MassGridAmountField::clear()
{
    amount->clear();
    unit->setCurrentIndex(0);
}

void MassGridAmountField::setEnabled(bool fEnabled)
{
    amount->setEnabled(fEnabled);
    unit->setEnabled(fEnabled);
}

bool MassGridAmountField::validate()
{
    bool valid = false;
    value(&valid);
    setValid(valid);
    return valid;
}

void MassGridAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("AmountSpinBox{\n height:60px;\n width:80px;\nborder:0px solid rgb(174,103,46,0);\n}\nAmountSpinBox::up-button\n{\n height: 0px;\nwidth:0px;\n}\nAmountSpinBox::down-button{\nheight: 0px;\nwidth:0px;\n}");
    else
        amount->setStyleSheet(STYLE_INVALID);
}

bool MassGridAmountField::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        // Clear invalid flag on focus
        setValid(true);
    }
    return QWidget::eventFilter(object, event);
}

QWidget *MassGridAmountField::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, amount);
    QWidget::setTabOrder(amount, unit);
    return unit;
}

CAmount MassGridAmountField::value(bool *valid_out) const
{
    return amount->value(valid_out);
}

void MassGridAmountField::setValue(const CAmount& value)
{
    amount->setValue(value);
}

void MassGridAmountField::setReadOnly(bool fReadOnly)
{
    amount->setReadOnly(fReadOnly);
}

void MassGridAmountField::unitChanged(int idx)
{
    // Use description tooltip for current unit for the combobox
    unit->setToolTip(unit->itemData(idx, Qt::ToolTipRole).toString());

    // Determine new unit ID
    int newUnit = unit->itemData(idx, MassGridUnits::UnitRole).toInt();

    amount->setDisplayUnit(newUnit);
}

void MassGridAmountField::setDisplayUnit(int newUnit)
{
    unit->setValue(newUnit);
}

void MassGridAmountField::setSingleStep(const CAmount& step)
{
    amount->setSingleStep(step);
}

QValueComboBox* MassGridAmountField::getUnitObject()
{
    return unit;
}
void MassGridAmountField::hideUnit()
{
    unit->hide();
}
