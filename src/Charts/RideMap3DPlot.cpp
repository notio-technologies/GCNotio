/*
 * Copyright (c) 2018 Michael Beaulieu
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

#include "RideMap3DPlot.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "IntervalItem.h"
#include "LTMWindow.h"
#include "TabView.h"
#include "GoldenCheetah.h"

#include <algorithm>

#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_picker.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>
#include <qwt_compat.h>

QMap<RideItem*, QMap<unsigned int, bool>> RideMap3DPlot::m_mapRideIntervalNewName;

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::RideMap3DPlot
///        Constructor.
///
/// \param[in] iContext Context.
/// \param[in] iParent  Object's parent.
///////////////////////////////////////////////////////////////////////////////////
RideMap3DPlot::RideMap3DPlot(Context *iContext, QWidget *iParent) : QwtPlot(iParent), m_context(iContext), m_parent(iParent)
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);
    setToolTip("");
    setToolTipDuration(2500);

    setXTitle();
    setAxesCount(QwtAxis::yLeft, 1);
    setAxesCount(QwtAxis::yRight, 1);

    // Hide Y axes.
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 0), false);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 0), false);

    double wPenWidth = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();

    // Current displayed metric.
    m_metricCurve = new QwtPlotCurve(tr("Current Metric"));
    m_metricCurve->setYAxis(QwtAxisId(QwtAxis::yLeft, 0));
    m_metricCurve->setPen(QPen(GColor(CCADENCE), wPenWidth));
    m_metricCurve->setZ(25);
    m_metricCurve->attach(this);

    // Set metric axis color.
    QPalette wPalette = palette();
    wPalette.setColor(QPalette::WindowText, GColor(CCADENCE));
    wPalette.setColor(QPalette::Text, GColor(CCADENCE));
    axisWidget(QwtAxisId(QwtAxis::yLeft, 0))->setPalette(wPalette);

    // Altitude Curve.
    m_altCurve = new QwtPlotCurve(tr("Altitude"));
    m_altCurve->setPen(QPen(GColor(CALTITUDE), wPenWidth));
    QColor wBrushColor = GColor(CALTITUDEBRUSH);
    wBrushColor.setAlpha(180);
    m_altCurve->setBrush(wBrushColor);
    m_altCurve->setYAxis(QwtAxisId(QwtAxis::yRight, 0));
    m_altCurve->attach(this);

    // Set altitude axis color
    wPalette.setColor(QPalette::WindowText, GColor(CRIDEPLOTYAXIS));
    wPalette.setColor(QPalette::Text, GColor(CRIDEPLOTYAXIS));
    axisWidget(QwtAxisId(QwtAxis::yRight, 0))->setPalette(wPalette);

    // Selected interval highligther.
    m_intervalSelection = new QwtPlotCurve();
    m_intervalSelection->setYAxis(QwtAxisId(QwtAxis::yRight, 0));
    m_intervalSelection->setZ(-20); // behind alt but infront of zones
    m_intervalSelection->attach(this);

    // Interval hover highlighter.
    m_intervalHover = new QwtPlotCurve();
    m_intervalHover->setYAxis(QwtAxisId(QwtAxis::yRight, 0));
    m_intervalHover->setZ(-20); // behind alt but infront of zones
    m_intervalHover->attach(this);

    // Tooltip displaying metric values of a curve.
    m_tooltip = new LTMToolTip(QwtPlot::xBottom, QwtAxisId(QwtAxis::yLeft, 0).id, QwtPicker::VLineRubberBand, QwtPicker::AlwaysOn, canvas(), "");
    m_tooltip->setRubberBand(QwtPicker::VLineRubberBand);
    m_tooltip->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    m_tooltip->setTrackerPen(QColor(Qt::black));
    QColor wInv(Qt::white);
    wInv.setAlpha(0);
    m_tooltip->setRubberBandPen(wInv);
    m_tooltip->setEnabled(true);

    // Curve marker.
    m_curveMarker = new QwtPlotMarker();
    m_curveMarker->setLineStyle(QwtPlotMarker::VLine);
    m_curveMarker->attach(this);
    m_curveMarker->setLabelAlignment(Qt::AlignCenter | Qt::AlignRight);
    m_curveMarker->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::SolidLine));

    // New selection markers.
    m_selMarker1 = new QwtPlotMarker();
    m_selMarker1->setLineStyle(QwtPlotMarker::VLine);
    m_selMarker1->attach(this);
    m_selMarker1->setLabelAlignment(Qt::AlignTop|Qt::AlignLeft);
    m_selMarker1->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

    m_selMarker2 = new QwtPlotMarker();
    m_selMarker2->setLineStyle(QwtPlotMarker::VLine);
    m_selMarker2->attach(this);
    m_selMarker2->setLabelAlignment(Qt::AlignBottom|Qt::AlignRight);
    m_selMarker2->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

    // Display tooltip and add curve marker.
    m_canvasPicker = new LTMCanvasPicker(this);
    connect(m_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*,int)), this, SLOT(pointHover(QwtPlotCurve*,int)));
    //connect(m_canvasPicker, SIGNAL(pointClicked(QwtPlotCurve*,int)), this, SLOT(pointClicked(QwtPlotCurve*,int)));

    // Manage new interval creation.
    connect(m_tooltip, SIGNAL(appended(QPoint)), this, SLOT(plotPickerSelected(QPoint)));
    connect(m_tooltip, SIGNAL(moved(QPoint)), this, SLOT(plotPickerMoved(QPoint)));
    connect(this, SIGNAL(simpleButtonClick()), this, SLOT(hideSelection()));
    connect(this, SIGNAL(dragNewSelection()), this, SLOT(plotPickerStarted()));
    connect(this, SIGNAL(endNewSelection()), this, SLOT(plotPickerStopped()));

    // Interval events.
    connect(m_context, SIGNAL(intervalSelected()), this, SLOT(refreshSelection()));
    connect(m_context, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
    connect(m_context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));
    connect(m_context, SIGNAL(configChanged(qint32)), SLOT(configChanged(qint32)));
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief The DataPoint struct
///        Datapoint structure for the time, altitude and current metric.
///////////////////////////////////////////////////////////////////////////////////
struct DataPoint
{
    double time, alt, metric;
    int inter;
    DataPoint(double t, double a, double m, int i) : time(t), alt(a), metric(m), inter(i) {}
};

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::recalc
///        This method recalculates the metrics curves.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::recalc()
{
    if (!m_timeArray.size()) return;

    long wRideTimeSecs = static_cast<long>(ceil(m_timeArray[m_arrayLength - 1]));
    if (wRideTimeSecs < 0 || wRideTimeSecs > SECONDS_IN_A_WEEK)
    {
        QwtArray<double> wData;
        m_metricCurve->setSamples(wData, wData);
        m_altCurve->setSamples(wData, wData);
        return;
    }

    double wTotalMetric = 0.0;
    double wTotalAlt = 0.0;
    QList<DataPoint*> wList;
    int i = 0;

    QVector<double> wSmoothMetric(wRideTimeSecs + 1);
    QVector<double> wSmoothAlt(wRideTimeSecs + 1);
    QVector<double> wSmoothTime(wRideTimeSecs + 1);

    QList<double> wInterList; //Just store the time that it happened.
    //Intervals are sequential.

    int wLastInterval = 0; //Detect if we hit a new interval

    for (int secs = 0; ((secs < m_smooth) && (secs < wRideTimeSecs)); ++secs)
    {
        wSmoothMetric[secs] = 0.0;
        wSmoothAlt[secs] = 0.0;
    }
    for (int secs = m_smooth; secs <= wRideTimeSecs; ++secs)
    {
        while ((i < m_arrayLength) && (m_timeArray[i] <= secs))
        {
            DataPoint *wDataPt = new DataPoint(m_timeArray[i], m_altArray[i], m_metricArray[i], m_interArray[i]);
            wTotalMetric += m_metricArray[i];
            wTotalAlt += m_altArray[i];

            wList.append(wDataPt);
            //Figure out when and if we have a new interval..
            if(wLastInterval != m_interArray[i])
            {
                wLastInterval = m_interArray[i];
                wInterList.append(secs/60.0);
            }
            ++i;
        }
        while (!wList.empty() && (wList.front()->time < secs - m_smooth))
        {
            DataPoint *wDataPt = wList.front();
            wList.removeFirst();
            wTotalMetric -= wDataPt->metric;
            wTotalAlt -= wDataPt->alt;

            delete wDataPt;
        }
        // TODO: this is wrong.  We should do a weighted average over the
        // seconds represented by each point...
        if (wList.empty())
        {
            wSmoothMetric[secs] = 0.0;
            wSmoothAlt[secs] = 0.0;
        }
        else
        {
            wSmoothMetric[secs] = wTotalMetric / wList.size();
            wSmoothAlt[secs] = wTotalAlt / wList.size();
        }
        wSmoothTime[secs] = secs / 60.0;
    }

    m_metricCurve->setSamples(wSmoothTime.constData(), wSmoothMetric.constData(), wRideTimeSecs + 1);
    m_altCurve->setSamples(wSmoothTime.constData(), wSmoothAlt.constData(), wRideTimeSecs + 1);

    setAxisScale(xBottom, 0.0, wSmoothTime.last());
    setYMax();
    refreshSelection();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setYMax
///        This method sets the Y axes maximum value based on the curves values.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setYMax()
{
    double wMetricMax = 0;
    double wMetricMin = 0;
    double wAltMax = 0;
    double wAltMin = 0;

    QString wAltLabel = "";
    QString wMetricLabel = "";

    // Get maxima and unit strings.
    bool wAltVisible = m_altCurve->isVisible();
    if (wAltVisible)
    {
        wAltMax = std::max(wAltMax, m_altCurve->maxYValue());

        wAltMin = *std::min_element(m_altArray.constBegin() + 1, m_altArray.constEnd());
        wAltLabel = m_altCurve->title().text() + " (m)";
    }
    else
    {
        wAltMax = 500;
    }
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 0), wAltVisible);

    bool wMetricVisible = m_metricCurve->isVisible();
    if (wMetricVisible)
    {
        wMetricMax = std::max(wMetricMax, m_metricCurve->maxYValue());
        wMetricMin = std::min(wMetricMin, m_metricCurve->minYValue());
        wMetricLabel = m_metricCurve->title().text();

        // Metric unit is defined.
        if (!m_metricUnit.isEmpty())
            wMetricLabel += " (" + m_metricUnit + ")";
    }
    else
    {
        wMetricMax = wAltMax;
    }
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 0), wMetricVisible);

    // Set maxima and axis titles.
    m_intervalMaxY = wAltMax * ((wAltMax < 0) ? 0.9 : 1.1);
    m_intervalMinY = wAltMin * ((wAltMin < 0) ? 1.1 : 0.9);

    setAxisScale(QwtAxisId(QwtAxis::yLeft, 0), wMetricMin * ((wMetricMin < 0) ? 1.1 : 0.9), wMetricMax * ((wMetricMax < 0) ? 0.9 : 1.1));
    setAxisTitle(QwtAxisId(QwtAxis::yLeft, 0), wMetricLabel);

    setAxisScale(QwtAxisId(QwtAxis::yRight, 0), m_intervalMinY, m_intervalMaxY);
    setAxisTitle(QwtAxisId(QwtAxis::yRight, 0), wAltLabel);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setXTitle
///        This method sets the title of the X axis. It also set the color.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setXTitle()
{
    setAxisTitle(xBottom, tr("Time (minutes)"));

    // Set axis color.
    QPalette wPalette = palette();
    wPalette.setColor(QPalette::WindowText, GColor(CRIDEPLOTXAXIS));
    wPalette.setColor(QPalette::Text, GColor(CRIDEPLOTXAXIS));

    axisWidget(xBottom)->setPalette(wPalette);

    enableAxis(xBottom, true);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setMetric
///        This method sets the Map 3D current metric.
///
/// \param[in] iSeriesSymbol    Metric data series.
/// ///////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setMetric(QString iSeriesSymbol)
{
    m_metricType = STANDARD;
    m_metricSeriesSymbol = iSeriesSymbol;
    m_metricUserData = nullptr;

    setCurrentMetricArray();
    recalc();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setMetric
///        This method sets the Map 3D current metric.
///
/// \param[in] iCustom  User data series.
/// ///////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setMetric(UserData *iCustom)
{
    m_metricType = USERDATA;
    m_metricUserData = iCustom;
    m_metricSeriesSymbol = "";

    setCurrentMetricArray();
    recalc();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setAxisTitle
///        This method sets an axis title.
///
/// \param[in] iAxisId   ID of the axis.
/// \param[in] iLabel    Label to be displayed.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setAxisTitle(QwtAxisId iAxisId, QString iLabel)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(nullptr, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(iLabel);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(iAxisId, stGiles);
    QwtPlot::setAxisTitle(iAxisId, title);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setData
///        This method sets the data of a ride item.
///
/// \param[in] iRideItem A ride item object.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setData(RideItem *iRideItem)
{
    if (iRideItem && iRideItem->ride())
    {
        m_currentRideItem = iRideItem;

        // Clear Selection marker and custom marker.
        clearSelection();

        // Update current user data series metric if valid.
        if ((m_metricType == USERDATA) && m_metricUserData)
            m_metricUserData->setRideItem(iRideItem);

        m_altArray.clear();
        m_timeArray.clear();
        m_interArray.clear();

        // Set metric arrays.
        m_arrayLength = 0;
        foreach (const RideFilePoint *point, m_currentRideItem->ride()->dataPoints())
        {
            m_timeArray.append(point->secs);
            m_altArray.append(point->alt);
            m_interArray.append(point->interval);
            ++m_arrayLength;
        }

        // Set the ride current metric data to be displayed.
        setCurrentMetricArray();

        recalc();
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setCurrentMetricArray
///        This method sets the ride current metric data selected on the Map 3D.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setCurrentMetricArray()
{
    // No ride selected.
    if (m_currentRideItem == nullptr)
        return;

    RideFile *wRide = m_currentRideItem->ride();
    bool wDisplayCurve = true;

    // Check if the array length has been set.
    if (m_arrayLength == 0)
    {
        m_arrayLength = wRide->dataPoints().size();
    }

    m_metricArray.clear();
    QString wToolTipText = "";
    double wPenWidth = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();

    RideFile::SeriesType wSeriesType = wRide->seriesForSymbol(m_metricSeriesSymbol);
    bool wSeriesPresent = wRide->isDataPresent(wSeriesType);

    // Verify if the current selected metric is a standard data series.
    if ((m_metricType == STANDARD) && wSeriesPresent)
    {
        m_metricUnit = wRide->unitName(wSeriesType, m_context);
        m_metricCurve->setTitle(wRide->seriesName(wSeriesType));
        m_metricCurve->setPen(QPen(wRide->colorFor(wSeriesType), wPenWidth));

        for (int i = 0; i < m_arrayLength; i++)
        {
            m_metricArray.append(wRide->dataPoints().at(i)->value(wSeriesType));
        }
    }

    // User Data.
    else if (m_metricUserData)
    {
        // Check that we have data.
        wSeriesPresent = false;
        for (auto &wDataItr : m_metricUserData->vector)
            wSeriesPresent |= (std::abs(wDataItr) > 0);

        if (wSeriesPresent)
        {
            m_metricCurve->setTitle(m_metricUserData->name);
            m_metricCurve->setPen(QPen(m_metricUserData->color, wPenWidth));

            // Get the metric unit string if it exist.
            m_metricUnit = m_metricUserData->units;

            // Get the datapoint for the entire ride.
            for (int i = 0; i < m_arrayLength; i++)
            {
                m_metricArray.append(m_metricUserData->vector[i]);
            }
        }
    }

    // No data to display.
    if (wSeriesPresent == false)
    {
        wToolTipText = tr("No metric data available.");
        wDisplayCurve = false;
        m_metricArray.resize(m_arrayLength);    // Need to be the size of m_arrayLength.
    }

    m_metricCurve->setVisible(wDisplayCurve);
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 0), wDisplayCurve);
    setToolTip(wToolTipText);

    // Reflect curve color.
    QPalette wPalette = palette();
    QPen wMetricColorPen = m_metricCurve->pen();
    wPalette.setColor(QPalette::WindowText, wMetricColorPen.color());
    wPalette.setColor(QPalette::Text, wMetricColorPen.color());
    axisWidget(QwtAxisId(QwtAxis::yLeft, 0))->setPalette(wPalette);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setSmoothing
///        This method sets the curves smoothing factor.
///
/// \param[in] iValue   Smoothing factor value.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setSmoothing(int iValue)
{
    m_smooth = iValue;
    recalc();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::intervalHover
///        This method highlights the current interval hovered by the mouse.
///
/// \param[in] iInterval    Interval hovered in the interval tree view of sidebar.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::intervalHover(IntervalItem *iInterval)
{
    // There's no point to do that.
    if (!isVisible() || iInterval == m_hovered) return;

    QVector<double> wDataX, wDataY;
    if (iInterval && iInterval->type != RideFileInterval::ALL) {

        // Hover curve color aligns to the type of interval we are highlighting
        QColor wHbrush = iInterval->color;
        wHbrush.setAlpha(64);
        m_intervalHover->setBrush(wHbrush);   // Fill with the color.

        wDataX << iInterval->start / 60;
        wDataY << m_intervalMinY;

        wDataX << iInterval->start / 60;
        wDataY << m_intervalMaxY;

        wDataX << iInterval->stop / 60;
        wDataY << m_intervalMaxY;

        wDataX << iInterval->stop / 60;
        wDataY << m_intervalMinY;

        m_intervalHover->setBaseline(m_intervalMinY);
    }

    // update state
    m_hovered = iInterval;

    m_intervalHover->setSamples(wDataX, wDataY);
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::refreshSelection
///        This method highlights the selected intervals.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::refreshSelection()
{
    if (m_currentRideItem == nullptr)
        return;

    QVector<double> wDataX, wDataY;
    bool wEntireRideSelected = m_currentRideItem->intervalsSelected(RideFileInterval::ALL).count();
    for (auto &wInterval : m_currentRideItem->intervalsSelected())
    {
        if (wInterval)
        {
            // Select curve color for highlighting.
            QColor hbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
            hbrush.setAlpha(128);
            m_intervalSelection->setBrush(hbrush);   // Fill with the color.

            wDataX << wInterval->start / 60;
            wDataY << m_intervalMinY;

            wDataX << wInterval->start / 60;
            wDataY << m_intervalMaxY;

            wDataX << wInterval->stop / 60;
            wDataY << m_intervalMaxY;

            wDataX << wInterval->stop / 60;
            wDataY << m_intervalMinY;

            m_intervalSelection->setBaseline(m_intervalMinY);

            // Highlight only the Entire Activity interval when it is selected.
            if ((wInterval->type == RideFileInterval::ALL) && wEntireRideSelected)
                break;
        }
    }

    // Check if the newest interval is still selected.
    if (m_newInterval && (m_newInterval->selected == false))
    {
        // Hide selection markers.
        hideSelection();
    }

    m_intervalSelection->setSamples(wDataX, wDataY);
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::intervalsChanged
///        This method is called when intervals changed. It refresh the list of new
///        selection and refresh the interval selection.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::intervalsChanged()
{
    if (m_currentRideItem == nullptr)
        return;

    QMap<RideItem*, QMap<unsigned int, bool> >::ConstIterator wItr = m_mapRideIntervalNewName.find(m_currentRideItem);
    if(wItr != m_mapRideIntervalNewName.end())
    {
        QList<QString> wIntervalNames;
        for(auto wInterval : m_currentRideItem->intervals())
        {
            wIntervalNames.append(wInterval->name);
        }

        QMap<unsigned int, bool> wValueMap = wItr.value();
        for(QMap<unsigned int, bool>::ConstIterator wValMapItr = wValueMap.begin(); wValMapItr != wValueMap.end(); ++wValMapItr)
        {
            unsigned int wTag = wValMapItr.key();
            QString wNameEx = QString(tr("Selection #%1 ")).arg(wTag);

            if(!wIntervalNames.contains(wNameEx))
            {
                wValueMap.insert(wTag, false);
            }
        }

        m_mapRideIntervalNewName.insert(m_currentRideItem, wValueMap);
    }
    refreshSelection();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setAltitudeVisible
///        This method shows/hides the altitude curve.
///
/// \param[in] iStatus  Visibility status.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setAltitudeVisible(const bool iStatus)
{
    m_altCurve->setVisible(iStatus);
    setAxisVisible(QwtAxisId(QwtAxis::yRight, 0), iStatus);
    setYMax();
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setMetricVisible
///        This method shows/hides the metric curve.
///
/// \param[in] iStatus  Visibility status.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setMetricVisible(const bool iStatus)
{
    m_metricCurve->setVisible(iStatus);
    setAxisVisible(QwtAxisId(QwtAxis::yLeft, 0), iStatus);
    setYMax();
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::pointHover
///        This method sets the text to be displayed beside the cursor while
///        hovering a curve coordinates.
///
/// \param[in] iCurve   Curve hovered.
/// \param[in] iIndex   Datapoint index.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::pointHover(QwtPlotCurve *iCurve, int iIndex)
{
    // Save graphic's tooltip text.
    static QString wPlotToolTipText;
    static bool wToolTipTextSaved = false;

    // Check if the curve is valid and we are not creating a new interval.
    if ((iIndex >= 0) && !m_selDrag && iCurve->isVisible())
    {
        double wValueY = iCurve->sample(iIndex).y();
        double wValueX = iCurve->sample(iIndex).x();

        // Determine metric sample display precision depending on min and max.
        int wPrecision = 1;
        if ((iCurve->maxYValue() < 0.1) && (iCurve->minYValue() > -0.1))
            wPrecision = 4;
        else if ((iCurve->maxYValue() < 2) && (iCurve->minYValue() > -2))
            wPrecision = 3;
        else if ((iCurve->maxYValue() < 50) && (iCurve->minYValue() > -50))
            wPrecision = 2;

        QTime wTimeX(0, 0, 0);

        // Output text of the tooltip.
        QString wText = QString("%1\n%2\n%3")
                .arg(this->axisTitle(iCurve->yAxis()).text())
                .arg(wValueY, 0, 'f', wPrecision)
                .arg(wTimeX.addSecs(static_cast<int>(wValueX * 60)).toString("hh:mm:ss"));

        // Set the curve tooltip text.
        m_tooltip->setText(wText);

        // Save graphic's tooltip text.
        if (!wToolTipTextSaved)
        {
            wPlotToolTipText = toolTip();
            wToolTipTextSaved = true;
        }

        // Remove graphic's tooltip text when hovering a curve.
        setToolTip("");
    }
    else
    {
        m_tooltip->setText("");

        // Restore graphic's tooltip text.
        if (wToolTipTextSaved)
        {
            setToolTip(wPlotToolTipText);
            wPlotToolTipText = "";
            wToolTipTextSaved = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::pointClicked
///        This method sets a custom marker on a curve upon clicking on it.
///
/// \param[in] iCurve   Curve hovered.
/// \param[in] iIndex   Datapoint index.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::pointClicked(QwtPlotCurve *iCurve, int iIndex)
{
    static bool wShowCustom = false;

    // Set and show curve marker.
    QwtText wText(QString(""));
    if (iIndex >= 0)
    {
        double wValueX = iCurve->sample(iIndex).x();
        if (!m_curveMarker->isVisible() || (std::abs(m_curveMarker->xValue() - wValueX) > 0))
        {
            QTime wTimeX(0, 0, 0);
            wText = QString("%1\n%2 %3\n%4")
                    .arg(iCurve->title().text())
                    .arg(iCurve->sample(iIndex).y(), 0, 'f', 2)
                    .arg(this->axisTitle(iCurve->yAxis()).text())
                    .arg(wTimeX.addSecs(static_cast<int>(wValueX * 60)).toString("hh:mm:ss"));
            wText.setFont(QFont("Helvetica", 10, QFont::Bold));
            wText.setColor(GColor(CPLOTMARKER));
            m_curveMarker->setLabel(wText);
            m_curveMarker->setValue(wValueX, 100);

            wShowCustom = true;
        }
        else
            wShowCustom = false;
    }
    m_curveMarker->setVisible(wShowCustom);
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::plotPickerSelected
///        This method sets the first marker position for the creation of a new
///        interval.
///
/// \param[in] iPos Point selected on the drawing region of the plot.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::plotPickerSelected(const QPoint &iPos)
{
    // Get the X axis plot value by converting the coordinates position from the drawing region.
    QwtPlotPicker* wPick = qobject_cast<QwtPlotPicker *>(sender());
    RideMap3DPlot* wPlot = qobject_cast<RideMap3DPlot *>(wPick->plot());
    double wValueX = wPlot->invTransform(QwtPlot::xBottom, iPos.x());

    // Set the selection marker 1 data.
    m_createNewInterval = true;
    m_selMarker1->setValue(wValueX, 100);

    QTime wTimeX(0, 0, 0);
    QwtText wText(QString("%1\n%2").arg(tr("Marker 1")).arg(wTimeX.addSecs(static_cast<int>(wValueX * 60)).toString("hh:mm:ss")));
    wText.setFont(QFont("Helvetica", 10, QFont::Bold));
    wText.setColor(GColor(CPLOTMARKER));
    m_selMarker1->setLabel(wText);

    // Hide it until a drag event occurs.
    m_selMarker1->hide();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::plotPickerMoved
///        This method gets the value of the second selection marker when the
///        Tooltip object is moved within the plot.
///
/// \param[in] iPos Cursor coordinates on the drawing region of the plot.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::plotPickerMoved(const QPoint &iPos)
{
    // Get the X axis plot value by converting the coordinates position from the drawing region.
    QwtPlotPicker* pick = qobject_cast<QwtPlotPicker *>(sender());
    RideMap3DPlot* plot = qobject_cast<RideMap3DPlot *>(pick->plot());
    double xValue = plot->invTransform(QwtPlot::xBottom, iPos.x());

    // The minimum interval length will be set to 30 seconds (0.5 minute).
    bool wNewInterval = (std::abs(m_selMarker1->xValue() - xValue) > kMinInterval);

    // Begin selecting a new interval.
    setSelection(xValue, wNewInterval);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::plotPickerStopped
///        This method removes the selection marker when the selection stop with
///        a time delta less than 30 seconds (0.5 minute).
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::plotPickerStopped()
{
    // Verify the length of the interval in minutes.
    if (abs(m_selMarker1->xValue() - m_selMarker2->xValue()) < kMinInterval)
    {
        hideSelection();
    }

    // Unblock the canvas picker signals.
    m_canvasPicker->blockSignals(false);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::plotPickerStarted
///        This method blocks any signal from the canvas picker to avoid displaying
///        the tooltip or adding a marker on the plot while creating a new interval.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::plotPickerStarted()
{
    // Don't display the tooltip.
    m_tooltip->setText("");

    // Block canvas picker signals.
    m_canvasPicker->blockSignals(true);
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::setSelection
///        This method creates a new interval and update it as long the drag event
///        is occuring.
///
/// \param[in] iValueX      Selection last X value read from the cursor.
/// \param[in] iNewInterval Indicates that we are creating a new interval.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::setSelection(double iValueX, bool iNewInterval)
{
    // Show the first marker since are selecting a new interval.
    m_selMarker1->show();

    // Set the second selection marker.
    if (!m_selMarker2->isVisible() || (std::abs(m_selMarker2->xValue() - iValueX) > 0))
    {
        m_selMarker2->setValue(iValueX, 100);

        QTime wTimeX(0, 0, 0);
        QwtText wText(QString("%1\n%2").arg(tr("Marker 2")).arg(wTimeX.addSecs(static_cast<int>(iValueX * 60)).toString("hh:mm:ss")));
        wText.setFont(QFont("Helvetica", 10, QFont::Bold));
        wText.setColor(GColor(CPLOTMARKER));
        m_selMarker2->setLabel(wText);
        m_selMarker2->show();

        double wX1, wX2;  // Time

        // Swap to low-high if neccessary depending on the relative position of the markers.
        if (m_selMarker1->xValue() > m_selMarker2->xValue())
        {
            wX2 = m_selMarker1->xValue();
            wX1 = m_selMarker2->xValue();
        }
        else
        {
            wX1 = m_selMarker1->xValue();
            wX2 = m_selMarker2->xValue();
        }

        double wDistance1 = -1;
        double wDistance2 = -1;
        double wDuration1 = -1;
        double wDuration2 = -1;

        // convert to seconds from minutes
        wX1 *= 60;
        wX2 *= 60;

        // Duration and distance.
        wDuration1 = wX1;
        wDuration2 = wX2;
        wDistance1 = m_currentRideItem->ride()->timeToDistance(wX1);
        wDistance2 = m_currentRideItem->ride()->timeToDistance(wX2);

        // Creating a new interval.
        if (iNewInterval)
        {
            // New interval doesn't exist yet.
            if(m_createNewInterval)
            {
                // Get the next tag value to add to new selection name.
                m_createNewInterval = false;
                unsigned int wNextTag = 0;
                if(m_mapRideIntervalNewName.contains(m_currentRideItem))
                {
                    wNextTag = getNextSelectionId(m_currentRideItem);
                }
                else
                {
                    wNextTag = 1;
                    QMap<unsigned int, bool> wMapIntervalNames;
                    wMapIntervalNames.insert(wNextTag, true);
                    m_mapRideIntervalNewName.insert(m_currentRideItem, wMapIntervalNames);
                }

                // Set default new interval name with the tag value.
                QString wNameEx = QString(tr("Selection #%1 ")).arg(wNextTag);

                // Create a new one.
                m_newInterval = m_currentRideItem->newInterval(wNameEx, wDuration1, wDuration2, wDistance1, wDistance2, Qt::black, false);

                // Select only the new interval.
                selectNewIntervalOnly();

                // Rebuild interval list for the current ride.
                m_context->notifyIntervalsUpdate(m_currentRideItem);
            }

            // Update new interval which is not done selecting.
            else
            {
                // New interval pointer is valid. Set new values.
                if(m_newInterval)
                {
                    m_newInterval->setValues(m_newInterval->name, wDuration1, wDuration2, wDistance1, wDistance2, m_newInterval->color, false);
                }
                else
                {
                    // Somethings wrong.
                    QMessageBox::warning(this, tr("Current interval null"), tr("Current interval null!!!!!"));
                }
            }

            // Charts need to update since new interval has changed.
            m_context->notifyIntervalsChanged();
        }
    }
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::clearSelection
///        This method clears selection markers and custom marker.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::clearSelection()
{
    checkIntervalsIntegrity();

    //qDebug()<<"clear selection";
    m_createNewInterval = false;

    // First ride selection, create list of generic selection intervals.
    if(m_currentRideItem && !m_mapRideIntervalNewName.contains(m_currentRideItem))
    {
        QMap<unsigned int, bool> wTags;
        for(auto wInterval : m_currentRideItem->intervals())
        {
            if(wInterval->name.startsWith(tr("Selection #")))
            {
                // Get tag number.
                QString name = wInterval->name;
                name = name.trimmed();
                QStringList listStr = name.split("#");
                wTags.insert(static_cast<uint>(listStr[1].toInt()), true);
            }
        }
        m_mapRideIntervalNewName.insert(m_currentRideItem, wTags);
    }

    // Hide markers.
    m_selMarker1->setVisible(false);
    m_selMarker2->setVisible(false);
    m_curveMarker->setVisible(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::configChanged
///        This method is called when something changed in the app
///        configuration.
///
/// \param[in] iConfig  Changed configuration flag.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::configChanged(qint32 iConfig)
{
    if (iConfig & CONFIG_APPEARANCE)
    {
        setProperty("color", GColor(CRIDEPLOTBACKGROUND));

#ifndef Q_OS_MAC
        setStyleSheet(TabView::ourStyleSheet());
#endif
        QPalette palette;

        palette.setColor(QPalette::Window, GColor(CRIDEPLOTBACKGROUND));
        palette.setColor(QPalette::Background, GColor(CRIDEPLOTBACKGROUND));
        palette.setBrush(QPalette::Window, GColor(CRIDEPLOTBACKGROUND));
        palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)));
        palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)));
        setPalette(palette);

        // Set plot background.
        setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));

        // Update curves appearence.
        if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true) {
            if (m_metricCurve)
                m_metricCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            if (m_altCurve)
                m_altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        }

        double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();

        if (m_metricCurve)
        {
            QPen wPen = m_metricCurve->pen();
            wPen.setWidthF(width);
            m_metricCurve->setPen(wPen);

            // Reflect curve color.
            QPalette wPalette;
            wPalette.setColor(QPalette::WindowText, wPen.color());
            wPalette.setColor(QPalette::Text, wPen.color());
            axisWidget(QwtAxisId(QwtAxis::yLeft, 0))->setPalette(wPalette);
        }
        if (m_altCurve)
        {
            QPen wPen = m_altCurve->pen();
            wPen.setWidthF(width);
            m_altCurve->setPen(wPen);
        }

        repaint();
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::hideSelection
///        This method hides the selection markers.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::hideSelection()
{
    m_selMarker1->setVisible(false);
    m_selMarker2->setVisible(false);
    replot();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::getNextSelectionId
///        This method get the next selection tag ID when creating a new interval.
///
/// \param[in] iCurrentRide Current ride.
/// \return A tag ID.
///////////////////////////////////////////////////////////////////////////////////
unsigned int RideMap3DPlot::getNextSelectionId(RideItem *iCurrentRide)
{
    unsigned int wToUse = 0;

    // Pass through the list of new intervals ID tag.
    if(m_mapRideIntervalNewName.contains(iCurrentRide))
    {
        QMap<unsigned int, bool> wValueMap = *(m_mapRideIntervalNewName.find(iCurrentRide));
        QMap<unsigned int, bool>::Iterator wItr = wValueMap.begin();
        while( wItr != wValueMap.end())
        {
            // Remove ID tags not used anymore.
            if(wItr.value() == false)
            {
                wItr = wValueMap.erase(wItr);
            }
            // Get the ID.
            else
            {
                wToUse = wItr.key();
                ++wItr;
            }
        }

        // Next ID value available.
        wToUse++;

        m_mapRideIntervalNewName.find(iCurrentRide)->insert(wToUse, true);
    }
    else
    {
        // create new value map
        QMap<unsigned int, bool> wMapIntervalNames;
        wToUse = 1;
        wMapIntervalNames.insert(wToUse, true);
        m_mapRideIntervalNewName.insert(iCurrentRide, wMapIntervalNames);
    }

    return wToUse;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::checkIntervalsIntegrity
///        This method verifies the integrity of the current ride intervals.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::checkIntervalsIntegrity()
{
    if (m_currentRideItem)
    {
        // Loop through the intervals.
        for(auto &wInterval : m_currentRideItem->intervals())
        {
            if ( wInterval->type == RideFileInterval::USER && !wInterval->rideInterval)
            {
                qDebug() << QString(tr("Warning: No ride interval saved for interval %1. Should save Ride after create/delete new intervals.")).arg(wInterval->name);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::selectNewIntervalOnly
///        This method selects only the new interval as soon the creation has
///        begun.
///////////////////////////////////////////////////////////////////////////////////
void RideMap3DPlot::selectNewIntervalOnly()
{
    // Loop through the interval.
    if (m_newInterval && m_currentRideItem)
    {
        for(auto &wInterval : m_currentRideItem->intervals())
        {
            // Deselect all but the new interval.
            wInterval->selected = (wInterval->name != m_newInterval->name) ? false : true;
        }
        m_context->notifyIntervalSelected();
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DPlot::event
///        This method is called when an event occurs.
///
/// \param[in] iEvent   Occuring event.
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool RideMap3DPlot::event(QEvent *iEvent)
{
    // Check for Mouse button release.
    static bool wMousePressed = false;
    if (iEvent->type() == QEvent::MouseButtonRelease)
    {
        // End selection if we were dragging the mouse on the plot.
        wMousePressed = false;
        if (m_selDrag)
        {
            m_selDrag = false;
            emit endNewSelection();
        }
        // We only click on the graph.
        else
        {
            emit simpleButtonClick();
        }
    }

    // Indicates we pressed the mouse button.
    if (iEvent->type() == QEvent::MouseButtonPress)
    {
        wMousePressed = true;
    }

    // Dragging if the mouse mouse moved after the button was pressed.
    if ((iEvent->type() == QEvent::MouseMove) && wMousePressed)
    {
        m_selDrag = true;
        emit dragNewSelection();
    }

    return QwtPlot::event(iEvent);
}
