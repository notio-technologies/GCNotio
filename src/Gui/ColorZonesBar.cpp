/*
 * Copyright (c) 2018 Michael Beaulieu (michael.beaulieu@notiotechnologies.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ColorZonesBar.h"
#include "Colors.h"

#include <QSizePolicy>

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::ColorZonesBar
///        Constructor.
///
/// \param[in] iColorList   Color zones list.
/// \param[in] iPointList   Division points list.
///////////////////////////////////////////////////////////////////////////////////
ColorZonesBar::ColorZonesBar(QList<QColor> iColorList, QList<double> iPointList)
{
    // There must be at least 2 zones else use default.
    if (iColorList.count() > 1)
    {
        m_colorList = iColorList;
        m_colorCnt = m_colorList.count();

        // Validate the points list and adjust it if needed.
        validatePoints(iPointList);
        m_pointList = iPointList;
    }

    CreateLayout();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::CreateLayout
///        This method creates the color zones bar layout.
///////////////////////////////////////////////////////////////////////////////////
void ColorZonesBar::CreateLayout()
{
    m_MainLayout = new QVBoxLayout(this);
    m_MainLayout->setContentsMargins(1, 1, 1, 1);

    QGridLayout *wColorBarLayout = new QGridLayout;
    wColorBarLayout->setContentsMargins(1, 1, 0, 1);

    // Minimum line indicator.
    QFrame *wLowIndicatorLine = new QFrame(this);
    wLowIndicatorLine->setFrameShape(QFrame::VLine);
    wLowIndicatorLine->setFrameShadow(QFrame::Sunken);
    wLowIndicatorLine->setLineWidth(1);
    wColorBarLayout->addWidget(wLowIndicatorLine, kColorBarRow, 0, Qt::AlignCenter | Qt::AlignBottom);

    // Setup the buttons to change zones color.
    m_zonesSelection = new QButtonGroup(this);
    int zonesColumn = 1;

    // Creates a button for each color.
    for (int i = 0; i < m_colorList.size(); i++, zonesColumn++)
    {
        ColorButton *wZoneButton = new ColorButton(this, QString("Range %1").arg(i+1), m_colorList[i]);

        m_zonesSelection->addButton(wZoneButton, i);
        wColorBarLayout->addWidget(wZoneButton, kColorBarRow, zonesColumn++, Qt::AlignCenter);

        // Add zone delimiters.
        QFrame *wZoneLine = new QFrame(this);

        wZoneLine->setFrameShape(QFrame::VLine);
        wZoneLine->setFrameShadow(QFrame::Sunken);
        wZoneLine->setFixedHeight(static_cast<int>(static_cast<double>(wZoneButton->height()) * kZoneDelimHeight * dpiYFactor));
        wZoneLine->setLineWidth(1);
        wColorBarLayout->addWidget(wZoneLine, kColorBarRow, zonesColumn, Qt::AlignCenter | Qt::AlignBottom);

        m_zoneButtons.append(wZoneButton);
        m_zoneLine.append(wZoneLine);
    }
    wLowIndicatorLine->setFixedHeight(static_cast<int>(static_cast<double>(m_zoneButtons[0]->height()) * kZoneDelimHeight * dpiYFactor));

    // High line indicator.
    QFrame *wHighIndicatorLine = m_zoneLine.last();

    // Add the color bar legend below.
    QLabel *m_lowLabel = new QLabel(tr("Lower"), this);
    wColorBarLayout->addWidget(m_lowLabel, kLegendRow, 0, Qt::AlignCenter);

    // Create a text box for each point.
    for (int i = 0; i < m_pointList.size(); i++)
    {
        ColorDivPointTextBox *wDelimiterLabel = new ColorDivPointTextBox(QLocale::system().toString(m_pointList[i], 'f', m_precision), this);
        QSizePolicy wTextSizePolicy;
        wTextSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
        wDelimiterLabel->setSizePolicy(wTextSizePolicy);
        wDelimiterLabel->setAlignment(Qt::AlignCenter);

#ifdef Q_OS_MAC
        wDelimiterLabel->setMaximumWidth(static_cast<int>(50 * dpiXFactor));
#else
        wDelimiterLabel->setMaximumWidth(static_cast<int>(60 * dpiXFactor));
#endif

        // Add a value validator to the text box.
        double wMinValue = (i > 0) ? (m_pointList[i - 1]) : std::numeric_limits<double>::lowest();
        double wMaxValue = (i < (m_colorList.size() - 2)) ? (m_pointList[i + 1]) : std::numeric_limits<double>::max();
        ColorDivPointValidator *wDelimiterValidator = new ColorDivPointValidator(wMinValue, wMaxValue, m_precision, wDelimiterLabel);
        wDelimiterValidator->setNotation(QDoubleValidator::StandardNotation);
        wDelimiterLabel->setValidator(wDelimiterValidator);

        // Align the text box with the zone delimiter line.
        sWidgetPosition wZoneDelimPos;
        wColorBarLayout->getItemPosition(wColorBarLayout->indexOf(m_zoneLine[i]), &(wZoneDelimPos.sRow), &(wZoneDelimPos.sColumn), &(wZoneDelimPos.sRowSpan), &(wZoneDelimPos.sColumnSpan));
        wColorBarLayout->addWidget(wDelimiterLabel, kLegendRow, wZoneDelimPos.sColumn, Qt::AlignCenter);

        m_delimiterLabel.append(wDelimiterLabel);
        m_delimiterValidator.append(wDelimiterValidator);

        connect(wDelimiterLabel, SIGNAL(textValidated()), this, SLOT(pointEdited()));
    }

    // Positioning of Higher label.
    if (wHighIndicatorLine)
    {
        sWidgetPosition wHighPosition;
        wColorBarLayout->getItemPosition(wColorBarLayout->indexOf(wHighIndicatorLine), &(wHighPosition.sRow), &(wHighPosition.sColumn), &(wHighPosition.sRowSpan), &(wHighPosition.sColumnSpan));
        QLabel *m_highLabel = new QLabel(tr("Higher"), this);
        wColorBarLayout->addWidget(m_highLabel, kLegendRow, wHighPosition.sColumn, Qt::AlignCenter);
    }

    m_MainLayout->addLayout(wColorBarLayout);

    connect(m_zonesSelection, SIGNAL(buttonClicked(int)), this, SLOT(changeColor(int)));
    connect(this, SIGNAL(pointChanged(int)), this, SLOT(changePoint(int)));
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::setColors
///        This method set the colors.
///
/// \param[in] iColors  Color zones list.
///
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool ColorZonesBar::setColors(QList<QColor> iColors)
{
    bool wReturning = false;

    // Check that the number of colors match the initial count.
    if (iColors.count() == m_colorCnt)
    {
        m_colorList.clear();
        m_colorList.append(iColors);
        refreshColors();
        wReturning = true;
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::setPoints
///        This method set the division points.
///
/// \param[in] iPoints  Division points list.
///
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool ColorZonesBar::setPoints(QList<double> iPoints)
{
    // Validates that the points list matches with the color zones.
    bool wReturning = validatePoints(iPoints);

    m_pointList.clear();
    m_pointList.append(iPoints);
    refreshPoints();

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::refreshPoints
///        This method refresh the zone division points list and their maximum and
///        minimum values.
///////////////////////////////////////////////////////////////////////////////////
void ColorZonesBar::refreshPoints()
{
    for (int i = 0; i < m_pointList.size(); i++)
    {
        // Set display text.
        m_delimiterLabel[i]->setText(QLocale::system().toString(m_pointList[i], 'f', m_precision));

        // Set new value range.
        double wMinValue = (i > 0) ? (m_pointList[i - 1]) : std::numeric_limits<double>::lowest();
        double wMaxValue = (i < (m_colorList.size() - 2)) ? (m_pointList[i + 1]) : std::numeric_limits<double>::max();

        m_delimiterValidator[i]->setRange(wMinValue, wMaxValue, m_precision);
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::refreshColors
///        This method refresh the zones color.
///////////////////////////////////////////////////////////////////////////////////
void ColorZonesBar::refreshColors()
{
    for (int i = 0; i < m_colorList.size(); i++)
    {
        m_zoneButtons[i]->setColor(m_colorList[i]);
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::validatePoints
///        This method validate that their is the right amount of points for the
///        quantity of color zones.
///
/// \param[in/out] iPointList   Division points list. Modified to fit the zones.
///
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool ColorZonesBar::validatePoints(QList<double> &iPointList)
{
    bool wReturning = false;
    int wPointCnt = iPointList.count();

    // There is always one division point less than the number of colors.
    if (m_colorCnt == (wPointCnt + 1))
    {
        wReturning = true;
    }

    // There are too much division points.
    else if (wPointCnt >= m_colorCnt)
    {
        int wPointToRem = wPointCnt - m_colorCnt + 1;

        for (int i = 0; i < wPointToRem; i++)
           iPointList.removeLast();
    }

    // Not enough division point compared to the number of colors.
    else
    {
        int wPointToAdd = m_colorCnt - wPointCnt - 1;

        for (int i = 0; i < wPointToAdd; i++)
            iPointList.append(iPointList.last() * 1.25);
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::changeColor
///        This method update a zone color.
///
/// \param[in] iIndex   Index of the zone.
///////////////////////////////////////////////////////////////////////////////////
void ColorZonesBar::changeColor(int iIndex)
{
    // Get the color.
    const QColor &wButtonColor = m_zoneButtons[iIndex]->getColor();

    // Update the color if modified.
    if (m_colorList[iIndex] != wButtonColor)
    {
        m_colorList.replace(iIndex, wButtonColor);
        emit colorBarUpdated(m_colorList);
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::changePoint
///        This methode update a division point value.
///
/// \param[in] iIndex   Index of the point.
///////////////////////////////////////////////////////////////////////////////////
void ColorZonesBar::changePoint(int iIndex)
{
    // Get the new value.
    const double wNewValue = QLocale::system().toDouble(m_delimiterLabel[iIndex]->text());

    // Update the value.
    m_pointList.replace(iIndex, wNewValue);

    // Refresh points validator.
    refreshPoints();

    emit colorBarUpdated(m_pointList);
}


///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorZonesBar::pointEdited
///        This method verify which division point has been modified.
///////////////////////////////////////////////////////////////////////////////////
void ColorZonesBar::pointEdited()
{
    for (int i = 0; i < m_pointList.size(); i++)
    {
        if (m_delimiterLabel[i]->isModified())
        {
            emit pointChanged(i);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorDivPointValidator::validate
///        This method validates the content of a color division point. It returns
///        either an invalid status or an acceptable status even if the value is
///        outside the range.
///
/// \param[in] iString  String to validate.
/// \param[in] iPos     Not used.
///
/// \return The state of the validation.
///////////////////////////////////////////////////////////////////////////////////
QValidator::State ColorDivPointValidator::validate(QString &iString, int &iPos) const
{
    // Validates
    QValidator::State wState = QDoubleValidator::validate(iString, iPos);

    // Invalid
    if (iString.isEmpty())
    {
        return wState;
    }
    else if (iString == "-")
    {
        return QValidator::Intermediate;
    }

    // Convert to a double.
    bool wOk = false;
    double wEntry = QLocale::system().toDouble(iString, &wOk);

    Q_UNUSED(wEntry)

    // Conversion OK.
    if (wOk)
    {
        return QValidator::Acceptable;
    }
    return wState;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief ColorDivPointValidator::truncatedValue
///        This method trunks the enter value of a division point text box when the
///        value entered is outside the range.
///////////////////////////////////////////////////////////////////////////////////
void ColorDivPointValidator::truncatedValue()
{
    // Get the parent.
    ColorDivPointTextBox* wLineEdit = dynamic_cast<ColorDivPointTextBox*>(parent());

    if (wLineEdit)
    {
        // Convert the string into a double.
        bool wOk = false;
        double wEntry = QLocale::system().toDouble(wLineEdit->text(), &wOk);

        if (wOk)
        {
            // Verify if the value is within the range.
            if (wEntry > this->top() || wEntry < this->bottom())
            {
                // Set the value to 10% below or above the infringed limit.
                double wTenPercentTop = (wEntry >= 0) ? kTopMargin : kBottomMargin;
                double wTenPercentBottom = (wEntry >= 0)? kBottomMargin : kTopMargin;

                wEntry = std::min<double>(wEntry, this->top() * wTenPercentTop);
                wEntry = std::max<double>(wEntry, this->bottom() * wTenPercentBottom);

                // Update the displayed text and set it has modified.
                wLineEdit->setText(QLocale::system().toString(wEntry, 'f', this->decimals()));
                wLineEdit->setModified(true);
            }

            emit wLineEdit->textValidated();
        }
    }
}
