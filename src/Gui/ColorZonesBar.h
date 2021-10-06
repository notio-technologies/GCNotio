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

#ifndef COLORZONESBAR_H
#define COLORZONESBAR_H

#include "ColorButton.h"

#include <QWidget>
#include <QDoubleValidator>
#include <QLocale>
#include <QValidator>
#include <QLineEdit>
#include <QFrame>
#include <QButtonGroup>

///////////////////////////////////////////////////////////////////////////////////
/// \brief The ColorDivPointValidator class
///        This class is a reimplementation of the QDoubleValidator class to add
///        the trunk feature on a value out-of-bound of a specific range.///
///////////////////////////////////////////////////////////////////////////////////
class ColorDivPointValidator : public QDoubleValidator
{
    Q_OBJECT

public:
    explicit ColorDivPointValidator(QObject *iParent = Q_NULLPTR) : QDoubleValidator(iParent)
    {
        connect(this->parent(), SIGNAL(editingFinished()), this, SLOT(truncatedValue()));
    }
    ColorDivPointValidator(double bottom, double top, int decimals, QObject *iParent = Q_NULLPTR) :
        QDoubleValidator(bottom, top, decimals, iParent)
    {
        connect(this->parent(), SIGNAL(editingFinished()), this, SLOT(truncatedValue()));
    }

    QValidator::State validate(QString &iString, int &iPos) const;

private slots:
    void truncatedValue();

private:
    static constexpr double kBottomMargin = 1.1;   // Bottom margin when truncating result.
    static constexpr double kTopMargin = 0.9;      // Top margin when truncating result.
};

///////////////////////////////////////////////////////////////////////////////////
/// \brief The ColorDivPointTextBox class
///        This class is a reimplementation of the QLineEdit class to add a new
///        signal method when a text entry has been validated by a
///        ColorDivPointValidator object.
///////////////////////////////////////////////////////////////////////////////////
class ColorDivPointTextBox : public QLineEdit
{
    Q_OBJECT

public:
    explicit ColorDivPointTextBox(QWidget *iParent = Q_NULLPTR) : QLineEdit(iParent) {}
    explicit ColorDivPointTextBox(const QString &iText, QWidget *iParent = Q_NULLPTR) : QLineEdit(iText, iParent) {}

signals:
    void textValidated();
};

///////////////////////////////////////////////////////////////////////////////////
/// \brief The ColorZonesBar class
///        This class create a color bar widget composed of different color zones.
///        Each zone is composed by a color and division points.
///        By default, the color bar is defined by 5 color zones but it could be
///        initialized with 2 or more zones. The number of division points is
///        determined by the number of colors minus 1.
///////////////////////////////////////////////////////////////////////////////////
class ColorZonesBar : public QWidget
{
    Q_OBJECT

public:
    // Default constructor.
    ColorZonesBar() { CreateLayout(); }
    ColorZonesBar(QList<QColor> iColorList, QList<double> iPointList);

    // Getters.
    QList<QColor> getColors() { return m_colorList; }
    QList<double> getPoints() { return m_pointList; }
    int getColorsCount() { return m_colorList.count(); }
    int getPointsCount() { return m_pointList.count(); }
    int getPrecision() { return m_precision; }

    // Setters.
    bool setColors(QList<QColor> iColors);
    bool setPoints(QList<double> iPoints);
    void setPrecision(int iDecimals) { m_precision = iDecimals; refreshPoints(); }

    // Constant.
    static const int kColorBarRow = 0;
    static const int kLegendRow = 1;
    static constexpr double kZoneDelimHeight = 1.25;

    struct sWidgetPosition
    {
        int sRow = 0;
        int sColumn = 0;
        int sRowSpan = 0;
        int sColumnSpan = 0;
    };

signals:
    void pointChanged(int);
    void colorChanged(int);
    void colorBarUpdated(const QList<QColor>);
    void colorBarUpdated(const QList<double>);

private slots:
    void changeColor(int);
    void pointEdited();
    void changePoint(int);
    void refreshPoints();
    void refreshColors();

private:
    void CreateLayout();
    bool validatePoints(QList<double> &iPointList);

    QVBoxLayout *m_MainLayout = nullptr;
    QList<ColorButton*> m_zoneButtons;
    QButtonGroup *m_zonesSelection = nullptr;
    QList<QFrame*> m_zoneLine;
    QList<ColorDivPointTextBox*> m_delimiterLabel;
    int m_precision = 2;
    QList<ColorDivPointValidator*> m_delimiterValidator;

    QList<double> m_pointList = { 50, 100, 150, 200 };
    QList<QColor> m_colorList = { QColor("#0000ff"), QColor("#00ff00"), QColor("#ffff00"), QColor("#ff5500"), QColor("#ff0000") };
    int m_colorCnt = 5;
};

#endif // COLORZONESBAR_H
