/*
 * Copyright (c) 2009 Andy M. Froncioni (me@andyfroncioni.com)
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

#include "NotioCDA.h"
#include "NotioCDAWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "RideItem.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#include "TimeUtils.h"
#include "Units.h"
#include "GcUpgrade.h"
#include "NotioComputeFunctions.h"
#include "AeroAlgo.h"

#include <cmath>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <set>
#include <QDebug>

#include <iostream>

using namespace std;
using namespace NotioComputeFunctions;
using namespace GcAlgo;

#define PI M_PI

#define MARC
static inline double
max(double a, double b) { if (a > b) return a; else return b; }
static inline double
min(double a, double b) { if (a < b) return a; else return b; }

static bool
isAlt(const RideFileDataPresent *dataPresent)
{
    return true;
}
static bool
isHeadWind(const RideFileDataPresent *dataPresent)
{
    return true;
}
static bool
isWatts(const RideFileDataPresent *dataPresent)
{
    return true;
}

/*----------------------------------------------------------------------
 * Interval plotting
 *--------------------------------------------------------------------*/

class IntervalNotioCDAData : public QwtSeriesData<QPointF>
{
public:
    NotioCDAPlot *aerolab;
    Context *context;
    IntervalNotioCDAData(NotioCDAPlot *aerolab, Context *context) : aerolab( aerolab ), context(context) { }

    double x( size_t ) const;
    double y( size_t ) const;
    size_t size() const;
    //virtual QwtData *copy() const;

    void init();

    IntervalItem *intervalNum( int ) const;

    int intervalCount() const;

    virtual QPointF sample(size_t i) const;
    virtual QRectF boundingRect() const;
};

/*
 * HELPER FUNCTIONS:
 *    intervalNum - returns a pointer to the nth selected interval
 *    intervalCount - returns the number of highlighted intervals
 */
// ------------------------------------------------------------------------------------------------------------
// note this is operating on the children of allIntervals and not the
// intervalWidget (QTreeWidget) -- this is why we do not use the
// selectedItems() member. N starts a one not zero.
// ------------------------------------------------------------------------------------------------------------
IntervalItem *IntervalNotioCDAData::intervalNum(int number) const
{
    if (aerolab->rideItem)
    {
        int highlighted=0;

        // Scan through all intervals.
        foreach(IntervalItem *p, aerolab->rideItem->intervals())
        {
            // Check if selected and if it is the specified interval.
            if (p->selected)
            {
                highlighted++;
            }

            // Return specified interval's pointer if selected.
            if (highlighted == number)
            {
                return p;
            }
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------------------------------------------------
// how many intervals selected?
// ------------------------------------------------------------------------------------------------------------
int IntervalNotioCDAData::intervalCount() const
{
    if (!aerolab->rideItem) return 0;

    int wHighlighted = 0;
    foreach(IntervalItem *p, aerolab->rideItem->intervals())
    {
        // Check if selected.
        if (p->selected)
        {
            wHighlighted++;
        }
    }

    return wHighlighted;
}
/*
 * INTERVAL HIGHLIGHTING CURVE
 * IntervalNotioCDAData - implements the qwtdata interface where
 *                    x,y return point co-ordinates and
 *                    size returns the number of points
 */
// The interval curve data is derived from the intervals that have
// been selected in the Context leftlayout for each selected
// interval we return 4 data points; bottomleft, topleft, topright
// and bottom right.
//
// the points correspond to:
// bottom left = interval start, 0 watts
// top left = interval start, maxwatts
// top right = interval stop, maxwatts
// bottom right = interval stop, 0 watts
//
double IntervalNotioCDAData::x(size_t number) const
{
    // for each interval there are four points, which interval is this for?
    // interval numbers start at 1 not ZERO in the utility functions

    double result = 0;

    int interval_no = number ? 1 + number / 4 : 1;
    // get the interval
    IntervalItem *current = intervalNum( interval_no );

    if ( current != NULL )
    {
        double multiplier = context->athlete->useMetricUnits ? 1 : static_cast<double>(MILES_PER_KM);

        // overlap at right ?
        double right = aerolab->bydist ? multiplier * current->stopKM : current->stop / 60;

        if ((number % 4 == 2) || (number % 4 == 3))
        {
            for (int n = 1; n <= intervalCount(); n++)
            {
                IntervalItem *other = intervalNum(n);
                if ((other != nullptr) && (other != current))
                {
                    if (other->start < current->stop && other->stop > current->stop)
                    {
                        if (other->start < current->start)
                        {
                            double _right = aerolab->bydist ? multiplier * current->startKM : current->start / 60;
                            if (_right<right)
                            {
                                right = _right;
                            }
                        }
                        else
                        {
                            double _right = aerolab->bydist ? multiplier * other->startKM : other->start / 60;
                            if (_right<right)
                            {
                                right = _right;
                            }
                        }
                    }
                }
            }
        }

        // which point are we returning?
        //qDebug() << "number = " << number << endl;
        switch ( number % 4 )
        {
        case 0 : result = aerolab->bydist ? multiplier * current->startKM : current->start/60; // bottom left
            break;
        case 1 : result = aerolab->bydist ? multiplier * current->startKM : current->start/60; // top left
            break;
        case 2 : result = right;  // bottom right
            break;
        case 3 : result = right;  // top right
            break;
        }
    }
    return result;
}
double IntervalNotioCDAData::y(size_t number) const
{
    // which point are we returning?
    double result = 0;
    switch ( number % 4 )
    {
    case 0 : result = -5000;  // bottom left
        break;
    case 1 : result = 5000;  // top left - set to out of bound value
        break;
    case 2 : result = 5000;  // top right - set to out of bound value
        break;
    case 3 : result = -5000;  // bottom right
        break;
    }
    return result;
}

size_t IntervalNotioCDAData::size() const
{
    return intervalCount() * 4;
}

QPointF IntervalNotioCDAData::sample(size_t i) const {
    return QPointF(x(i), y(i));
}

QRectF IntervalNotioCDAData::boundingRect() const
{
    return QRectF(-5000, 5000, 10000, 10000);
}

//**********************************************
//**        END IntervalNotioCDAData           **
//**********************************************


NotioCDAPlot::NotioCDAPlot(NotioCDAWindow *parent, Context *context) : QwtPlot(parent),
    context(context), parent(parent), rideItem(NULL), smooth(1), bydist(true),
    //NKC1
    //NKC2
    GarminON(false), WindOn(false)
{
    // Set Y axes count.
    setAxesCount(QwtPlot::yLeft, 1);
    setAxesCount(QwtPlot::yRight, 2);

    //Axis debug
    //setAxesCount(QwtPlot::yRight, 3);

    crr       = 0.005;
    cda       = 0.500;
    cdaRange       = 0.300;
    totalMass = 85.0;
    riderFactor       = 1.234;
    riderExponent     = 4.321;
    eta       = 1.0;
    windowSize   = 60.0; // In constructor
    cdaWindowSize   = 60.0; // In constructor
    constantAlt = false;

    insertLegend(new QwtLegend(this), QwtPlot::BottomLegend);

    setCanvasBackground(Qt::white);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    QwtPlot::setAxisTitle(cCdaAxisLeft, tr("CdA"));
    setAxisScale(cCdaAxisLeft, cda-cdaRange, cda+cdaRange);

    QwtPlot::setAxisTitle(cAltAxisRight, QString(tr("Altitude (%1)")).arg(context->athlete->useMetricUnits ? "m" : tr("feet")));
    setAxisScale(cAltAxisRight, -50, 50);

    QwtPlot::setAxisTitle(cWindAxisRight, QString(tr("Wind (%1)")).arg(context->athlete->useMetricUnits ? tr("kph") : tr("mph")));
    setAxisScale(cWindAxisRight, -10, 10);

    //QwtPlot::setAxisTitle(QwtAxisId(cDebugAxisRight), tr(""));
    //setAxisScale(cDebugAxisRight, -500, 500);

    setAxisVisible(cCdaAxisLeft, true);
    setAxisVisible(cAltAxisRight, true);
    setAxisVisible(cWindAxisRight, WindOn);

    //NKC1
    //NKC2

    // CdA curve
    cdaAverageCurve = new QwtPlotCurve(tr("Avg CdA"));
    cdaAverageCurve->attach(this);

    // Altitude curves
    altCurve = new QwtPlotCurve(tr("Corrected Elevation"));
    altCurve->attach(this);
    altInCurve = new QwtPlotCurve(tr("Elevation"));
    altInCurve->attach(this);
    altHeadUnitCurve = new QwtPlotCurve(tr("HU Elevation"));
    altHeadUnitCurve->attach(this);

    // Wind curves
    windCurve = new QwtPlotCurve(tr("Wind"));
    windCurve->attach(this);
    windAverageCurve = new QwtPlotCurve(tr("Avg Estimated Wind"));
    windAverageCurve->attach(this);
    vWindCurve = new QwtPlotCurve(tr("Estimated Wind"));
    vWindCurve->attach(this);

    // Create reference markers of the wind and CdA offest.
    windRef = new QwtPlotMarker(tr("Wind Zero Reference"));
    windRef->setLabelAlignment(Qt::AlignRight);
    windRef->setYAxis(cWindAxisRight);
    windRef->setLineStyle(QwtPlotMarker::HLine);
    windRef->setYValue(0.0);
    windRef->setVisible(WindOn);
    windRef->attach(this);

    cdaOffsetRef = new QwtPlotMarker(tr("CdA Offset"));
    cdaOffsetRef->setLabelAlignment(Qt::AlignLeft);
    cdaOffsetRef->setYAxis(cCdaAxisLeft);
    cdaOffsetRef->setLineStyle(QwtPlotMarker::HLine);
    cdaOffsetRef->setYValue(cda);
    cdaOffsetRef->attach(this);
    cdaOffsetRef->setVisible(true);

    //NKC1
    //NKC2

    // Set CdA curve axis.
    cdaAverageCurve->setYAxis(cCdaAxisLeft);

    // Set altitude curves axis.
    altCurve->setYAxis(cAltAxisRight);
    altInCurve->setYAxis(cAltAxisRight);
    altHeadUnitCurve->setYAxis(cAltAxisRight);

    // Set wind curves axis.
    windCurve->setYAxis(cWindAxisRight);
    windAverageCurve->setYAxis(cWindAxisRight);
    vWindCurve->setYAxis(cWindAxisRight);

    intervalHighlighterCurve = new QwtPlotCurve();
    intervalHighlighterCurve->setBaseline(-5000);
    intervalHighlighterCurve->setYAxis( cCdaAxisLeft );
    intervalHighlighterCurve->setSamples( new IntervalNotioCDAData( this, context ) );
    intervalHighlighterCurve->attach( this );
    static_cast<QwtLegend*>(legend())->legendWidget(itemToInfo(intervalHighlighterCurve))->setVisible(false); // don't show in legend

    m_intervalHoverCurve = new QwtPlotCurve();
    m_intervalHoverCurve->setYAxis(cCdaAxisLeft);
    m_intervalHoverCurve->setBaseline(-20); // go below axis
    m_intervalHoverCurve->setZ(-20); // behind alt but infront of zones
    m_intervalHoverCurve->attach(this);
    static_cast<QwtLegend*>(legend())->legendWidget(itemToInfo(m_intervalHoverCurve))->setVisible(false); // don't show in legend
    connect(context, SIGNAL(intervalHover(IntervalItem*)), SLOT(intervalHover(IntervalItem*)));

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    axisWidget(cAltAxisRight)->installEventFilter(this);;
    axisWidget(cCdaAxisLeft)->installEventFilter(this);;
    axisWidget(cWindAxisRight)->installEventFilter(this);;
    axisWidget(QwtAxisId(QwtPlot::xBottom, 0))->installEventFilter(this);
    if (legend()) legend()->installEventFilter(this);

    configChanged(CONFIG_APPEARANCE);
}


void
NotioCDAPlot::configChanged(qint32)
{
    QPalette palette;
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setBrush(QPalette::Window, GColor(CRIDEPLOTBACKGROUND));
    QColor wLegendTextColor = GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND));

    setPalette(palette);

    // set colors
    setCanvasBackground(GColor(CRIDEPLOTBACKGROUND));

    //NKC1

    //NKC2

    if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true) {
        windCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        windAverageCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        cdaAverageCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        vWindCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        altCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        altInCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        altHeadUnitCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        windRef->setRenderHint(QwtPlotItem::RenderAntialiased);
        cdaOffsetRef->setRenderHint(QwtPlotItem::RenderAntialiased);
    }

    // Wind curve color and line width update.
    QPen windPen = QPen(GColor(CPLOTMARKER));
    windPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    windCurve->setPen(windPen);

    // Get curve title and set color for the legend.
    QwtText wTitle = windCurve->title();
    wTitle.setColor(wLegendTextColor);
    windCurve->setTitle(wTitle);

    // Average wind curve color and line width update.
    QPen windAveragePen = QPen(GColor(CWINDSPEED));
    windAveragePen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    windAverageCurve->setPen(windAveragePen);

    // Get curve title and set color for the legend.
    wTitle = windAverageCurve->title();
    wTitle.setColor(wLegendTextColor);
    windAverageCurve->setTitle(wTitle);

    // Average CdA curve color and line width update.
    QPen cdaAveragePen = QPen(GColor(CPOWER));
    cdaAveragePen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    cdaAverageCurve->setPen(cdaAveragePen);

    // Get curve title and set color for the legend.
    wTitle = cdaAverageCurve->title();
    wTitle.setColor(wLegendTextColor);
    cdaAverageCurve->setTitle(wTitle);

    // Virtual wind curve color and line width update.
    QPen vWindPen = QPen(GColor(CAEROVE));
    vWindPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    vWindCurve->setPen(vWindPen);

    // Get curve title and set color for the legend.
    wTitle = vWindCurve->title();
    wTitle.setColor(wLegendTextColor);
    vWindCurve->setTitle(wTitle);    

    // Corrected altitude curve color and line width update.
    QPen altPen = QPen(GColor(CAEROEL));
    altPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    altCurve->setPen(altPen);

    // Get curve title and set color for the legend.
    wTitle = altCurve->title();
    wTitle.setColor(wLegendTextColor);
    altCurve->setTitle(wTitle);

    // Original altitude curve color and line width update.
    QPen altInPen = QPen(GColor(CGEAR));
    altInPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    altInPen.setStyle(Qt::DashDotLine);
    altInCurve->setPen(altInPen);

    // Get curve title and set color for the legend.
    wTitle = altInCurve->title();
    wTitle.setColor(wLegendTextColor);
    altInCurve->setTitle(wTitle);

    // Head unit altitude curve color and line width update.
    QPen altHeadUnitPen = QPen(GColor(PALTCOLOR));
    altHeadUnitPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    altHeadUnitPen.setStyle(Qt::DashDotDotLine);
    altHeadUnitCurve->setPen(altHeadUnitPen);

    // Get curve title and set color for the legend.
    wTitle = altHeadUnitCurve->title();
    wTitle.setColor(wLegendTextColor);
    altHeadUnitCurve->setTitle(wTitle);

    // Reference markers color and line settings.
    QPen windZeroPen = QPen(GColor(CPLOTMARKER));
    windZeroPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    windZeroPen.setStyle(Qt::PenStyle::DashDotDotLine);
    windRef->setLinePen(windZeroPen);
    QwtText windRefText = QString(tr("Wind Ref%1")).arg("\n0 " + (context->athlete->useMetricUnits ? tr("kph") : tr("mph")));
    QColor windBackGroundColor = GColor(CPLOTMARKER);
    windBackGroundColor.setAlpha(cColorAlpha);
    windRefText.setColor(GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)));
    windRefText.setBackgroundBrush(QBrush(windBackGroundColor));
    windRef->setLabel(windRefText);

    QPen cdaZeroPen = QPen(GColor(CZONE1));
    cdaZeroPen.setWidthF(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    cdaZeroPen.setStyle(Qt::PenStyle::DashDotDotLine);
    cdaOffsetRef->setLinePen(cdaZeroPen);
    setCdaOffsetMarker(cda);

    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);

    QPen ihlPen = QPen( GColor( CINTERVALHIGHLIGHTER ) );
    ihlPen.setWidth(1);
    intervalHighlighterCurve->setPen( ihlPen );

    QColor ihlbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
    ihlbrush.setAlpha(40);
    intervalHighlighterCurve->setBrush(ihlbrush);   // fill below the line

    m_intervalHoverCurve->setPen(QPen(Qt::NoPen));
    QColor hbrush = QColor(Qt::lightGray);
    hbrush.setAlpha(64);
    m_intervalHoverCurve->setBrush(hbrush);   // fill below the line

    // Update axes text and color.	
	// X axis.
    setXTitle();

    // CdA Y axis.
    palette.setColor(QPalette::WindowText, GColor(CPOWER));
    palette.setColor(QPalette::Text, GColor(CPOWER));
    palette.setBrush(QPalette::Window, GColor(CPOWER));
    axisWidget(cCdaAxisLeft)->setPalette(palette);

    // Altitude Y axis.
    palette.setColor(QPalette::WindowText, GColor(CAEROEL));
    palette.setColor(QPalette::Text, GColor(CAEROEL));
    palette.setBrush(QPalette::Window, GColor(CAEROEL));
    axisWidget(cAltAxisRight)->setPalette(palette);
    QwtPlot::setAxisTitle(cAltAxisRight, QString(tr("Altitude (%1)")).arg(context->athlete->useMetricUnits ? "m" : tr("feet")));

    // Wind Y axis.
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setBrush(QPalette::Window, GColor(CPLOTMARKER));
    axisWidget(cWindAxisRight)->setPalette(palette);
    QwtPlot::setAxisTitle(cWindAxisRight, QString(tr("Wind (%1)")).arg(context->athlete->useMetricUnits ? tr("kph") : tr("mph")));

    // Debug Y axis.
    //palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    //palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    //palette.setBrush(QPalette::Window, GColor(CPLOTMARKER));
    //axisWidget(cDebugAxisRight)->setPalette(palette);

    // Update legend.
    updateLegend();
    legend()->setVisible(m_showLegend);

    repaint();
}

static int supercount = 0;

void
NotioCDAPlot::setData(RideItem *_rideItem, bool new_zoom) {

    // HARD-CODED DATA: p1->kph
    double m       =  totalMass;

    rideItem = _rideItem;
    RideFile *ride = rideItem->ride();

    if ( NotioCDAData.setData(ride) == 0 )
        return ;

    if ( windowSize > 0 ) {
        cout << "Set window to " << windowSize << endl;
        NotioCDAData.m_Crr = crr;
        NotioCDAData.m_MechEff = eta;
        NotioCDAData.m_TotalWeight = totalMass;
        NotioCDAData.m_RiderFactor = riderFactor;
        NotioCDAData.m_RiderExponent = riderExponent;
        NotioCDAData.m_InertiaFactor = inertiaFactor;
    }

    //NKC1

    //NKC2

    windArray.clear();
    windAverageArray.clear();
    cdaAverageArray.clear();
    vWindArray.clear();
    syncArray.clear();
    altArray.clear();
    altInArray.clear();
    altHeadUnitArray.clear();
    distanceArray.clear();
    timeArray.clear();

    if (ride && ride->xdata("CDAData")) {

        XDataSeries *wCdaDataSeries = ride->xdata("CDAData");
        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        //setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

        if( isWatts(dataPresent) ) {

            // If watts are present, then we can fill the rollingCDAArray data:
            const RideFileDataPresent *dataPresent = ride->areDataPresent();
            int npoints = NotioCDAData.getNumPoints(); //ride->dataPoints().size();
            double dt = 1.0;//ride->recIntSecs(); //CDAData is in seconds and recInt looks at Samples, not XDATA
            //NKC1
            //NKC2
            windArray.resize(isWatts(dataPresent) ? npoints : 0);
            windAverageArray.resize(isWatts(dataPresent) ? npoints : 0);
            cdaAverageArray.resize(isWatts(dataPresent) ? npoints : 0);
            vWindArray.resize(isWatts(dataPresent) ? npoints : 0);
            syncArray.resize(isWatts(dataPresent) ? npoints : 0);
            altArray.resize(isAlt(dataPresent) || constantAlt ? npoints : 0);
            altInArray.resize(isAlt(dataPresent) || constantAlt ? npoints : 0);
            altHeadUnitArray.resize(isAlt(dataPresent) || constantAlt ? npoints : 0);
            timeArray.resize(isWatts(dataPresent) ? npoints : 0);
            distanceArray.resize(isWatts(dataPresent) ? npoints : 0);

            {
                for ( int i = 0 ; i < npoints ; i++ )
                    syncArray[i] = 0.0;
                foreach(IntervalItem *interval, rideItem->intervals()) {
                    //        	    	if (interval->type != RideFileInterval::USER) continue;
                    if (interval->selected) {
                        qDebug() << "Interval selected" << interval->name ;
                        qDebug() << "Interval start" << interval->start;
                        int idx2 = wCdaDataSeries->timeIndex(interval->start);
                        syncArray[idx2] = 1.0;
                    }
                }
            }

            // quickly erase old data

            //NKC1
            //NKC2
            windAverageCurve->setVisible(false);
            windCurve->setVisible(false);
            vWindCurve->setVisible(false);
            windRef->setVisible(false);
            altCurve->setVisible(false);
            altInCurve->setVisible(false);
            altHeadUnitCurve->setVisible(false);

            //NKC1

            //NKC2

            QwtPlot::setAxisVisible(QwtAxisId(yRight, 1), WindOn);

            // detach and re-attach the CdA curve:
            cdaAverageCurve->detach();
            if (!cdaAverageArray.empty()) {
                cdaAverageCurve->attach(this);
                cdaAverageCurve->setVisible((isWatts(dataPresent)) && rideItem->intervalsSelected().count());
            }

            // detach and re-attach the Corrected Elevation curve:
            bool have_recorded_alt_curve = false;
            altCurve->detach();
            if (!altArray.empty()) {
                have_recorded_alt_curve = true;
                altCurve->attach(this);
                altCurve->setVisible(isAlt(dataPresent) || constantAlt );
            }

            bool wDebugOn = ride->getTag("notio.debug","off").contains("on");

            // detach and re-attach the Elevation curve:
            altInCurve->detach();
            if (!altInArray.empty() && wDebugOn) {
                altInCurve->attach(this);
                if ( GarminON == true )
                    altInCurve->setVisible(isWatts(dataPresent));
            }

            // detach and re-attach the Head Unit Elevation curve:
            altHeadUnitCurve->detach();
            if (!altHeadUnitArray.empty() && wDebugOn) {
                altHeadUnitCurve->attach(this);
                if ( GarminON == true )
                    altHeadUnitCurve->setVisible(isWatts(dataPresent));
            }

            // detach and re-attach the Estimated Wind curve:
            vWindCurve->detach();
            if (!vWindArray.empty()) {
                if ( WindOn == true )
                {
                    vWindCurve->attach(this);
                    vWindCurve->setVisible(isWatts(dataPresent));
                }
            }

            // detach and re-attach the Wind curve:
            windCurve->detach();
            if (!windArray.empty()) {
                if ( WindOn == true )
                {
                    windCurve->attach(this);
                    windCurve->setVisible(isWatts(dataPresent));
                }
            }

            // detach and re-attach the Average Wind curve:
            windAverageCurve->detach();
            if (!windAverageArray.empty()) {
                if (WindOn == true)
                {
                    windAverageCurve->attach(this);
                    if (rideItem->intervalsSelected().count())
                    {
                        windAverageCurve->setVisible(isWatts(dataPresent));
                    }
                }
            }

            // detach and re-attach the Wind Reference marker:
            windRef->detach();
            if (WindOn == true) {
                windRef->attach(this);
                windRef->setVisible(isWatts(dataPresent));
            }

            // Fill the virtual elevation profile with data from the ride data:
            //double t = 0.0;
            arrayLength = 0;

            int _numPoints =NotioCDAData.getNumPoints();


            //FILE *veLog = fopen("/Users/marcg/Dropbox/temp/veLog.csv","w");

            for( int p1Index = 0 ; p1Index < _numPoints ; p1Index++ ) {
                // If this is first element, set e to eoffser

                timeArray[arrayLength]  = NotioCDAData.getSecs(p1Index) / 60.0;

                if ( have_recorded_alt_curve ) {
                    if ( constantAlt && arrayLength > 0) {
                        altArray[arrayLength] = altArray[arrayLength-1];
                        altInArray[arrayLength] = altArray[arrayLength-1]; //FIXME
                        altHeadUnitArray[arrayLength] = altArray[arrayLength-1]; //FIXME
                    }
                    else  {
                        if ( constantAlt && !isAlt(dataPresent)) {
                            altArray[arrayLength] = 0;
                            altInArray[arrayLength] = 0; //FIXME
                            altHeadUnitArray[arrayLength] = 0; //FIXME
                        }
                        else {
                            int use = p1Index + timeOffset;

                            if ( use < 0 )
                                use = 0;
                            else if ( use >= npoints )
                                use = npoints-1;

                            altArray[arrayLength] = (context->athlete->useMetricUnits
                                                     ? NotioCDAData.getCorrectedAlt(use)
                                                     : NotioCDAData.getCorrectedAlt(use) * static_cast<double>(FEET_PER_METER));
                            altInArray[arrayLength] = (context->athlete->useMetricUnits
                                                       ? NotioCDAData.getOriginalAlt(p1Index)
                                                       : NotioCDAData.getOriginalAlt(p1Index) * static_cast<double>(FEET_PER_METER));
                            altHeadUnitArray[arrayLength] = (context->athlete->useMetricUnits
                                                             ? NotioCDAData.getHeadUnitAlt(p1Index)
                                                             : NotioCDAData.getHeadUnitAlt(p1Index) * static_cast<double>(FEET_PER_METER));
                        }
                    }
                }
                else {
                    cout << "P1Index " << p1Index << endl;
                }

                // Unpack:
                distanceArray[arrayLength] = ride->timeToDistance(NotioCDAData.getSecs(p1Index)) * static_cast<double>(context->athlete->useMetricUnits ? 1 : MILES_PER_KM);
                double power = max(0.0, NotioCDAData.getWatts(p1Index));

                //NKC1
                //NKC2

                windArray[arrayLength] = NotioCDAData.getWind(p1Index, NotioCDAData::WINDAVG);
                windAverageArray[arrayLength] = 0.0;
                cdaAverageArray[arrayLength] = cda;
                vWindArray[arrayLength] = NotioCDAData.getVirtualWind(p1Index,riderFactor,riderExponent, NotioCDAData::WINDAVG);

                ++arrayLength;
            }
            //fclose(veLog);
        } else {
            //NKC1

            //NKC2
            windCurve->setVisible(false);
            windAverageCurve->setVisible(false);
            cdaAverageCurve->setVisible(false);
            vWindCurve->setVisible(false);
            windRef->setVisible(false);
            altCurve->setVisible(false);
            altInCurve->setVisible(false);
            altHeadUnitCurve->setVisible(false);
        }

        cout << "Time to recalc " << supercount++ << endl;

        recalc(new_zoom);
        cout << "post recalc" << endl;
        adjustEoffset();
        cout << "post recalcdone " << endl;
    }
    else {
        //setTitle("no data");

    }
}

void
NotioCDAPlot::setAxisTitle(int axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);

}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAPlot::rideSelected
///        This method sets the currently selected RideItem object and updates
///        the rideFile in NotioCDAData.
///
/// \param[in] iRideItem    Selected RideItem.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAPlot::rideSelected(RideItem *iRideItem)
{
    rideItem = iRideItem;
    if (rideItem == nullptr)
        return;
    NotioCDAData.setData(rideItem->ride());
}

void
NotioCDAPlot::adjustEoffset() {

    return;

}


struct DataPoint {
    double time, hr, watts, speed, cad, alt;
    DataPoint(double t, double h, double w, double s, double c, double a) :
        time(t), hr(h), watts(w), speed(s), cad(c), alt(a) {}
};

void
NotioCDAPlot::recalc( bool new_zoom ) {

    if (timeArray.empty())
        return;

    int rideTimeSecs = (int) ceil(timeArray[arrayLength - 1]);

    // If the ride is really long, then avoid it like the plague.
    if (rideTimeSecs > SECONDS_IN_A_WEEK) {
        QVector<double> data;

        //NKC1

        //NKC2

        if (!windArray.empty()){
            windCurve->setSamples(data, data);
        }
        if (!windAverageArray.empty()){
            windAverageCurve->setSamples(data, data);
        }
        if (!cdaAverageArray.empty()){
            cdaAverageCurve->setSamples(data, data);
        }
        if (!vWindArray.empty()){
            vWindCurve->setSamples(data, data);
        }
        if( !altArray.empty()) {
            altCurve->setSamples(data, data);
        }
        if( !altInArray.empty()) {
            altInCurve->setSamples(data, data);
        }
        if( !altHeadUnitArray.empty()) {
            altHeadUnitCurve->setSamples(data, data);
        }
        return;
    }

    if (rideItem && rideItem->ride() && rideItem->ride()->xdata("CDAData"))
    {
        XDataSeries *wCdaDataSeries = rideItem->ride()->xdata("CDAData");
        int counter = 0;
        double grandTotal = 0.0;

        for ( int i = 0 ; i < arrayLength ; i++ )
            windAverageArray[i] = -10000.0;

        foreach(IntervalItem *interval, rideItem->intervals()) {

            if (interval->selected) {
                int idx2 = wCdaDataSeries->timeIndex(interval->start);
                int idx3 = wCdaDataSeries->timeIndex(interval->stop);
                double cda = NotioCDAData.intervalCDA(idx2, idx3);

                double vWindTotal = 0.0;
                double aWindTotal = 0.0;
                for (int j = idx2 ; j <idx3 ; j++) {
                    vWindTotal += vWindArray[j];
                    aWindTotal += windArray[j];
                }
                if (idx3 > idx2) {
                    vWindTotal /= (idx3-idx2);
                    aWindTotal /= (idx3-idx2);
                }

                cout << "vWindAverage " << vWindTotal << endl;
                cout << "aWindAverage " << aWindTotal << endl;

                for (int j = idx2; j < idx3; j++) {
                    windAverageArray[j] = vWindTotal;
                    cdaAverageArray[j] = std::max(std::min(cda, 1.575), -1.575);
                }

                grandTotal += vWindTotal;
                counter++;
            }
        }

        if ( counter > 0 )
            grandTotal /= counter;

        for ( int i = 0 ; i < arrayLength ; i++ ) {
            if ( windAverageArray[i] < -9999.99 )
                windAverageArray[i]  = grandTotal;
        }
    }


    QVector<double> &xaxis = (bydist?distanceArray:timeArray);
    int startingIndex = 0;
    int totalPoints   = arrayLength - startingIndex;

    // set curves

    //NKC1
    //NKC2
    if (!windArray.empty()) {
        windCurve->setSamples(xaxis.data() + startingIndex, windArray.data() + startingIndex, totalPoints);
    }
    if (!windAverageArray.empty()) {
        windAverageCurve->setSamples(xaxis.data() + startingIndex, windAverageArray.data() + startingIndex, totalPoints);
    }
    if (!cdaAverageArray.empty()) {
        cdaAverageCurve->setSamples(xaxis.data() + startingIndex, cdaAverageArray.data() + startingIndex, totalPoints);
    }
    if (!vWindArray.empty()) {
        vWindCurve->setSamples(xaxis.data() + startingIndex, vWindArray.data() + startingIndex, totalPoints);
    }
    if (!altArray.empty()){
        altCurve->setSamples(xaxis.data() + startingIndex, altArray.data() + startingIndex, totalPoints);
    }
    if (!altInArray.empty()){
        altInCurve->setSamples(xaxis.data() + startingIndex, altInArray.data() + startingIndex, totalPoints);
    }
    if (!altHeadUnitArray.empty()){
        altHeadUnitCurve->setSamples(xaxis.data() + startingIndex, altHeadUnitArray.data() + startingIndex, totalPoints);
    }

    if( new_zoom )
        setAxisScale(QwtAxisId(QwtPlot::xBottom, 0), 0.0, (bydist? distanceArray[arrayLength - 1] : timeArray[arrayLength - 1]));

    setYMax(new_zoom );
    refreshIntervalMarkers();

    replot();
}

void
NotioCDAPlot::setYMax(bool new_zoom)
{
    Q_UNUSED(new_zoom)

    double minY = -50;
    double maxY = 50;

    //NKC1
    if (!altArray.empty())
    {
        minY = altCurve->minYValue();
        minY *= ((minY < 0) ? 1.1 : 0.9);
        maxY = altCurve->maxYValue() * 1.1;
        maxY *= ((maxY < 0) ? 0.9 : 1.1);
    }
    setAxisScale(cAltAxisRight, minY, maxY);

    minY = -10;
    maxY = 10;
    if (!windArray.empty())
    {
        minY = windCurve->minYValue();
        minY *= ((minY < 0) ? 1.1 : 0.9);
        maxY = windCurve->maxYValue() * 1.1;
        maxY *= ((maxY < 0) ? 0.9 : 1.1);
    }
    setAxisScale(cWindAxisRight, minY, maxY);

    setAxisScale(cCdaAxisLeft, cda-cdaRange, cda+cdaRange);
    QwtPlot::setAxisTitle(cCdaAxisLeft, tr("CdA") );

    //NKC1

}

void
NotioCDAPlot::setXTitle() {

    QString wTitle;
    if (bydist)
         wTitle = QString(tr("Distance ")) + QString(context->athlete->useMetricUnits? "(km)" : "(miles)");
    else
        wTitle = tr("Time (minutes)");

    QwtPlot::setAxisTitle(QwtAxisId(QwtPlot::xBottom, 0), wTitle);

    QPalette palette;
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    axisWidget(QwtAxisId(QwtPlot::xBottom, 0))->setPalette(palette);
}

//NKC1

void
NotioCDAPlot::setConstantAlt(int value)
{
    constantAlt = value;
}

//NKC2

void
NotioCDAPlot::setGarminON(int value)
{
    GarminON = (bool)value;
}

void
NotioCDAPlot::setWindOn(int value)
{
    WindOn = (bool)value;
}

void
NotioCDAPlot::setByDistance(int value) {
    bydist = value;
    setXTitle();
    recalc(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAPlot::setShowLegend
///        This method is called when we want to show a legend.
///
/// \param[in] iState   State of the legend setting.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAPlot::setShowLegend(bool iState)
{
    m_showLegend = iState;
    if (legend())
    {
        updateLegend();
        legend()->setVisible(iState);
    }
}

// At slider 1000, we want to get max Crr=0.1000
// At slider 1    , we want to get min Crr=0.0001
void
NotioCDAPlot::setIntCrr(int value) {

    crr = (double) value / 1000000.0;

    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntCda(int value)  {
    cda = (double) value / 10000.0;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntCdaRange(int value)  {
    cdaRange = (double) value / 10000.0;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntTotalMass(int value) {

    totalMass = (double) value / 100.0;
    recalc(false);
}

void NotioCDAPlot::setCdaOffsetMarker(double iCda)
{
    cdaOffsetRef->setYValue(iCda);
    QwtText cdaOffsetText = QString(tr("CdA Offset%1")).arg("\n" + QString::number(iCda, 'f', 3));
    QColor cdaBackgroundColor = GColor(CZONE1);
    cdaBackgroundColor.setAlpha(cColorAlpha);
    cdaOffsetText.setColor(GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)));
    cdaOffsetText.setBackgroundBrush(QBrush(cdaBackgroundColor));
    cdaOffsetRef->setLabel(cdaOffsetText);
    cdaOffsetRef->setVisible(true);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntRiderFactor(int value) {

    riderFactor = (double) value / 10000.0;
    cout << "Set Int RiderFactor " << riderFactor << endl;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntRiderExponent(int value) {

    riderExponent = (double) value / 10000.0;
    cout << "Set Int RiderExponent " << riderExponent << endl;
    recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntEta(int value) {

    eta = (double) value / 10000.0;
    recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntCalcWindow(int value) {
    windowSize = (double) value;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntCDAWindow(int value) {
    cdaWindowSize = (double) value;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntTimeOffset(int value) {
    if ( value < -10.0 || value > 10.0 )
        return;

    timeOffset = (double)value;
    cout << "--------------------------   Time offset " << timeOffset << endl;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioCDAPlot::setIntInertiaFactor(int value) {
    inertiaFactor = (double)value / 10000.0;
    cout << "--------------------------   inertia Factor" << inertiaFactor << endl;
    recalc(false);
}

void NotioCDAPlot::pointHover (QwtPlotCurve *curve, int index)
{
    if ( index >= 0 && curve != intervalHighlighterCurve && curve->isVisible() )
    {
        double x_value = curve->sample( index ).x();
        double y_value = curve->sample( index ).y();

        // Determine metric sample display precision depending on min and max.
        int wPrecision = 1;
        if ((curve->maxYValue() < 0.1) && (curve->minYValue() > -0.1))
            wPrecision = 4;
        else if ((curve->maxYValue() < 2) && (curve->minYValue() > -2))
            wPrecision = 3;
        else if ((curve->maxYValue() < 50) && (curve->minYValue() > -50))
            wPrecision = 2;

        double wMultiplier = (context->athlete->useMetricUnits ? 1 : static_cast<double>(MILES_PER_KM));
        bool wWithinSelectedIntervals = false;

        // Verify if the current point is contained withing selected intervals.
        for (auto &wInterval : rideItem->intervalsSelected())
        {
            double wLowerBound = bydist ? (wInterval->startKM * wMultiplier) : (wInterval->start / 60);
            double wUpperBound = bydist ? (wInterval->stopKM * wMultiplier) : (wInterval->stop / 60);

            if ((x_value > wLowerBound) && (x_value < wUpperBound))
            {
                wWithinSelectedIntervals = true;
                break;
            }
        }

        // Get curve title.
        QString wTooltipCurveTitle = curve->title().text();

        // Average CdA and Average Wind curves have a special title in tooltip outside selected intervals.
        if ((curve == cdaAverageCurve) && !wWithinSelectedIntervals)
        {
            // CdA curve takes the scale offset value.
            wTooltipCurveTitle = tr("CdA Offset");
        }
        // Wind curve values are the average of the selected intervals.
        else if (curve == windAverageCurve)
        {
            if ((rideItem->intervalsSelected().count() > 1) && !wWithinSelectedIntervals)
            {
                wTooltipCurveTitle.prepend(tr("Selected intervals:") + "\n");
            }
            // Put interval name only if one is selected.
            else if (rideItem->intervalsSelected().count() == 1)
            {
                wTooltipCurveTitle.prepend(rideItem->intervalsSelected().at(0)->name + ":\n");
            }
        }

        // Get X axis title.
        QString wAxisX = this->axisTitle(curve->xAxis()).text();

        // X axis displays distance.
        if (bydist)
        {
            // Insert value and remove parenthesis around units.
            wAxisX.insert(wAxisX.indexOf('(', 0), QString::number(x_value, 'f', 3));
            wAxisX.replace('(', ' ');
            wAxisX.remove(')');
        }
        // X axis displays time.
        else
        {
            QTime wTimeX(0, 0, 0);
            wAxisX = wTimeX.addSecs(static_cast<int>(x_value * 60)).toString("hh:mm:ss");
        }

        // Get opening parenthesis index from the string.
        QString wAxisY = this->axisTitle(curve->yAxis()).text();
        int wUnitIndex = wAxisY.indexOf('(', 0);

        // The metric doesn't have a unit.
        if (wUnitIndex < 0)
            wUnitIndex = wAxisY.length();

        // Remove Y axis curve name and keep only the units.
        wAxisY = wAxisY.remove(0, wUnitIndex);

        // Insert metric value and remove parenthesis around units.
        wAxisY.prepend(QString::number(y_value, 'f', wPrecision));
        wAxisY.replace('(', ' ');
        wAxisY.remove(')');

        // output the tooltip
        QString text = QString( "%1 %2 %3" )
                . arg(wTooltipCurveTitle + "\n")
                . arg(wAxisY + "\n")
                . arg(wAxisX);

        // set that text up
        tooltip->setText( text );
    }
    else
    {
        // no point
        tooltip->setText( "" );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAPlot::intervalHover
///        This method is called whenever an interval is hovered in the
///        interval section from the sidebar. It highlights it with his defined
///        color.
///
/// \param[in] iInterval    Interval hovered pointer.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAPlot::intervalHover(IntervalItem *iInterval)
{
    // There's no point to do that.
    if (!isVisible() || iInterval == m_hovered) return;

    double wMultiplier = (context->athlete->useMetricUnits ? 1 : static_cast<double>(MILES_PER_KM));

    QVector<double> wDataX, wDataY;
    if (iInterval && iInterval->type != RideFileInterval::ALL) {

        // Hover curve color aligns to the type of interval we are highlighting
        QColor wHbrush = iInterval->color;
        wHbrush.setAlpha(64);
        m_intervalHoverCurve->setBrush(wHbrush);   // Fill with the color.

        wDataX << (bydist ? iInterval->startKM * wMultiplier : iInterval->start / 60);
        wDataY << axisScaleDiv(cCdaAxisLeft).lowerBound();

        wDataX << (bydist ? iInterval->startKM * wMultiplier : iInterval->start / 60);
        wDataY << axisScaleDiv(cCdaAxisLeft).upperBound();

        wDataX << (bydist ? iInterval->stopKM * wMultiplier : iInterval->stop / 60);
        wDataY << axisScaleDiv(cCdaAxisLeft).upperBound();

        wDataX << (bydist ? iInterval->stopKM * wMultiplier : iInterval->stop / 60);
        wDataY << axisScaleDiv(cCdaAxisLeft).lowerBound();
    }

    // update state
    m_hovered = iInterval;

    m_intervalHoverCurve->setSamples(wDataX, wDataY);
    QwtIndPlotMarker::resetDrawnLabels();
    replot();
}

void NotioCDAPlot::refreshIntervalMarkers()
{
    //cout << "Refresh Interval Markers" << endl;
    QwtIndPlotMarker::resetDrawnLabels();
    double wMultiplier = (context->athlete->useMetricUnits ? 1 : static_cast<double>(MILES_PER_KM));

    foreach( QwtPlotMarker *mrk, d_mrk )
    {
        mrk->detach();
        delete mrk;
    }
    d_mrk.clear();

    //QRegExp wkoAuto("^(Peak *[0-9]*(s|min)|Entire workout|Find #[0-9]*) *\\([^)]*\\)$");
    if ( rideItem->ride() )
    {
        foreach(IntervalItem *interval, rideItem->intervals()) {

            // only plot user intervals
            if (interval->type == RideFileInterval::USER)
            {
                QwtPlotMarker *mrk = new QwtIndPlotMarker;
                d_mrk.append(mrk);
                mrk->attach(this);
                mrk->setLineStyle(QwtPlotMarker::VLine);
                mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
                mrk->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));

                QwtText text(interval->name);
                text.setFont(QFont("Helvetica", 10, QFont::Bold));
                text.setColor(GColor(CPLOTMARKER));
                if (!bydist) {
                    //cout << "Interval Start time" << interval->start << endl;
                    mrk->setValue(interval->start / 60.0, 0.0);
                }
                else {
                    //cout << "Interval Start km " << interval->startKM << endl;
                    mrk->setValue(wMultiplier * interval->startKM, 0.0);
                }
                mrk->setLabel(text);
            }

            // Add a textless marker to indicate the end of a selected interval.
            if ((interval->type != RideFileInterval::ALL) && interval->selected)
            {
                // Its main purpose is to see the delimitations of an interval which overlap another.
                QwtPlotMarker *mrkEnd = new QwtIndPlotMarker;
                d_mrk.append(mrkEnd);
                mrkEnd->attach(this);
                mrkEnd->setLineStyle(QwtPlotMarker::VLine);
                mrkEnd->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
                mrkEnd->setLinePen(QPen(GColor(CINTERVALHIGHLIGHTER), 0, Qt::DashDotLine));
                QwtText text("");
                text.setFont(QFont("Helvetica", 10, QFont::Bold));
                text.setColor(GColor(CPLOTMARKER));

                if (!bydist) {
                    mrkEnd->setValue(interval->stop / 60.0, 0.0);
                }
                else {
                    mrkEnd->setValue(wMultiplier * interval->stopKM, 0.0);
                }
                mrkEnd->setLabel(text);
            }
        }
    }
    //cout << "Refresh Interval Markers done" << endl;
}

/*
 * Estimate CdA and Crr usign energy balance in segments defined by
 * non-zero altitude.
 * Returns an explanatory error message ff it fails to do the estimation,
 * otherwise it updates cda and crr and returns an empty error message.
 * Author: Alejandro Martinez
 * Date: 23-aug-2012
 */
QString NotioCDAPlot::estimateCdACrr(RideItem *rideItem)
{
    // HARD-CODED DATA: p1->kph
    const double vfactor = 3.600;
    const double g = KG_FORCE_PER_METER;
    RideFile *ride = rideItem->ride();
    QString errMsg;

    //setupEstimateAltitudes(rideItem);
    //return errMsg;


    if(ride) {
        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        if(( isAlt(dataPresent) || constantAlt )  && isWatts(dataPresent)) {
            double dt = 1.0;//ride->recIntSecs();
            int npoints = ride->dataPoints().size();
#ifdef Q_CC_MSVC
            double* X1 = new double[npoints];
            double* X2 = new double[npoints];
            double* Egain = new double[npoints];
#else
            double X1[npoints], X2[npoints], Egain[npoints];
#endif
            int nSeg = -1;
            double altInit = 0, vInit = 0;
            /* For each segment, defined between points with alt != 0,
             * this loop computes X1, X2 and Egain to verify:
             * Aero-Loss + RR-Loss = Egain
             * where
             *      Aero-Loss = X1[nSgeg] * CdA
             *      RR-Loss = X2[nSgeg] * Crr
             * are the aero and rr components of the energy loss with
             *      X1[nSeg] = sum(0.5 * rho * headwind*headwind * distance)
             *      X2[nSeg] = sum(totalMass * g * distance)
             * and the energy gain sums power in the segment with
             * potential and kinetic variations:
             *      Egain = sum(eta * power * dt) +
             *              totalMass * (g * (altInit - alt) +
             *              0.5 * (vInit*vInit - v*v))
             */
            int _numPoints = NotioCDAData.getNumPoints();
            for( int p1Index = 0 ; p1Index < _numPoints ; p1Index++ ) {
                // Unpack:
                double power = max(0.0, NotioCDAData.getWatts(p1Index));
                double v     = NotioCDAData.getKPH(p1Index)/vfactor;
                double distance = v * dt;
#ifdef MARC
                double DP = NotioCDAData.getDP(p1Index);
                if ( DP  < 0.0 )
                    DP = 0.0;
                else
                    DP = DP * std::pow(AeroAlgo::calculateWindFactor(1.39, -0.05, DP), 2.0);
#else
                double headwind = v;
                if( isHeadWind(dataPresent) ) {
                    headwind   = p1->headwind/vfactor;
                }
#endif
                double alt = NotioCDAData.getCorrectedAlt(p1Index);
                // start initial segment
                if (nSeg < 0 && alt != 0) {
                    nSeg = 0;
                    X1[nSeg] = X2[nSeg] = Egain[nSeg] = 0.0;
                    altInit = alt;
                    vInit = v;
                }
                // accumulate segment data
                if (nSeg >= 0) {
                    // X1[nSgeg] * CdA == Aero-Loss
#ifdef MARC
                    X1[nSeg] += DP * distance;
#else
#endif
                    // X2[nSgeg] * Crr == RR-Loss
                    X2[nSeg] += totalMass * g * distance;
                    // Energy supplied
                    Egain[nSeg] += eta * power * dt;
                }
                // close current segment and start a new one
                if (nSeg >= 0 && alt != 0) {
                    // Add change in potential and kinetic energy
                    Egain[nSeg] += totalMass * (g * (altInit - alt) + 0.5 * (vInit*vInit - v*v));
                    // Start a new segment
                    nSeg++;
                    X1[nSeg] = X2[nSeg] = Egain[nSeg] = 0.0;
                    altInit = alt;
                    vInit = v;
                }
            }
            /* At least two segmentes needed to approximate:
             *     X1 * CdA + X2 * Crr = Egain
             * which, in matrix form, is:
             *    X * [ CdA ; Crr ] = Egain
             * and pre-multiplying by X transpose (X'):
             *    X'* X [ CdA ; Crr ] = X' * Egain
             * which is a system with two equations and two unknowns
             *    A * [ CdA ; Crr ] = B
             */
            if (nSeg >= 2) {
                // compute the normal equation: A * [ CdA ; Crr ] = B
                // A = X'*X
                // B = X'*Egain
                double A11 = 0, A12 = 0, A21 = 0, A22 = 0, B1 = 0, B2 = 0;
                for (int i = 0; i < nSeg; i++) {
                    A11 += X1[i] * X1[i];
                    A12 += X1[i] * X2[i];
                    A21 += X2[i] * X1[i];
                    A22 += X2[i] * X2[i];
                    B1  += X1[i] * Egain[i];
                    B2  += X2[i] * Egain[i];
                }
                // Solve the normal equation
                // A11 * CdA + A12 * Crr = B1
                // A21 * CdA + A22 * Crr = B2
                double det = A11 * A22 - A12 * A21;
                if (fabs(det) > 0.00) {
                    // round and update if the values are in NotioCDA's range
                    double cda = floor(10000 * (A22 * B1 - A12 * B2) / det + 0.5) / 10000;
                    double crr = floor(1000000 * (A11 * B2 - A21 * B1) / det + 0.5) / 1000000;
                    if (cda >= 0.001 && cda <= 1.0 && crr >= 0.0001 && crr <= 0.1) {
                        this->cda = cda;
                        this->crr = crr;
                        errMsg = ""; // No error
                    } else {
                        errMsg = tr("Estimates out-of-range");
                        cout << "cda " << cda << endl;
                        cout << "crr " << crr << endl;
                    }
                } else {
                    errMsg = tr("At least two segments must be independent");
                }
            } else {
                errMsg = tr("At least two segments must be defined");
            }
#ifdef Q_CC_MSVC
            delete[] X1;
            delete[] X2;
            delete[] Egain;
#endif
        } else {
            errMsg = tr("Altitude and Power data must be present");
        }
    } else {
        errMsg = tr("No activity selected");
    }
    return errMsg;
}

typedef struct {
    int start;
    int end;
    double wind;
} interval;

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAPlot::estimateFactExp
///        This method calculates the coefficient 1 based on the wind of the
///        currently selected calibration intervals.
///
/// \param[in]  rideItem    Current RideItem.
/// \param[in]  iExponent   Stick factor.
/// \param[out] oFactor     Coefficient 1
///
/// \return An error message.
///////////////////////////////////////////////////////////////////////////////
QString NotioCDAPlot::estimateFactExp(RideItem *rideItem, const double &iExponent, double &oFactor)
{
    RideFile *wRide = (rideItem != nullptr) ? rideItem->ride() : nullptr;
    QString wErrMsg;
    QString wInstructions = tr(" - 1 interval for interior velodrome (Beta).\n");
    wInstructions.append(tr(" - 2 intervals; outbound and inbound.\n"));
    wInstructions.append(tr(" - An even number of outbound and inbound intervals."));

    if (wRide)
    {
        int wIntrvlCnt = rideItem->intervalsSelected().count();

        XDataSeries *wRideDataSeries = wRide->xdata("RideData");
        bool wOldFormat = (wRide->getTag("Gc Min Version", "0").toInt() <= NK_VERSION_LATEST);

        // Check for errors.
        if (wRideDataSeries == nullptr)
        {
            wErrMsg = QString(tr("No RideData available. Compute ride first."));
        }
        else if (wIntrvlCnt < 1)
        {
            wErrMsg = QString(tr("No interval selected.\n"));
            wErrMsg.append(wInstructions);
        }
        else if ((wIntrvlCnt > 1) && (wIntrvlCnt % 2 != 0))
        {
            wErrMsg = QString(tr("Odd number of intervals selected.\n"));
            wErrMsg.append(wInstructions);
        }
        else
        {
            // Get data series.
            std::vector<double> wSpeedData, wPressureData, wDensityData;
            std::vector<Utility::sIntervalRange> wIntervalList;

            // Get data column numbers.
            int wAirPressureIndex = wRideDataSeries->valuename.indexOf("airpressure");
            int wAirDensityIndex = wRideDataSeries->valuename.indexOf("airdensity");
            int wSpeedIndex = wRideDataSeries->valuename.indexOf("speed");

            if ((wAirPressureIndex > -1) && (wAirDensityIndex > -1) && (wSpeedIndex > -1))
            {
                // Create vector containing necessary data to estimate the calibration factor.
                for (auto &wPointItr : wRideDataSeries->datapoints)
                {
                    wSpeedData.push_back(wPointItr->number[wSpeedIndex] * Utility::cMeterPerSecToKph);
                    wPressureData.push_back(wPointItr->number[wAirPressureIndex] / (wOldFormat ? GcAlgo::AeroAlgo::cAirPressureSensorFactor : 1.0));
                    wDensityData.push_back(wPointItr->number[wAirDensityIndex]);
                }

                // Get selected intervals start and stop indexes.
                for (auto &wIntervalItr : rideItem->intervalsSelected())
                {
                    int wIdx2 = wRideDataSeries->timeIndex(wIntervalItr->start);
                    int wIdx3 = wRideDataSeries->timeIndex(wIntervalItr->stop);

                    wIntervalList.push_back({ wIdx2, wIdx3, 0.0 });
                }

                // Estimate calibration factor.
                oFactor = GcAlgo::AeroAlgo::estimate_calibration_factor(wIntervalList, wPressureData, wDensityData, wSpeedData, iExponent);
            }
            else
                wErrMsg = tr("Data missing. Cannot estimate calibration factor. Please try to recompute.");
        }
    }
    else
    {
        wErrMsg = tr("No activity selected");
    }

    return wErrMsg;
}

NotioCDAData::NotioCDAData()
{
    rawCDAIndex = -1;
    winCDAIndex = -1;
    ekfAltIndex = -1;
    bcvAltIndex = -1;
    huAltIndex = -1;
    windIndex = -1;
    airpressureIndex = -1;
    airDensityIndex = -1;
    eTotIndex = -1;
    speedIndex = -1;
    ekfCDAIndex = -1;
    dataPoints = -1;
}

int
NotioCDAData::setData(RideFile *inRide)
{
    thisRide = inRide;
    if ( thisRide != NULL ) {
        cdaSeries  = thisRide->xdata("CDAData"); //MG
        if ( cdaSeries != NULL ) {
            dataPoints = cdaSeries->datapoints.count();
            for (int a = 0; a < cdaSeries->valuename.count(); a++) {
                if (cdaSeries->valuename.at(a) == "speed")
                    speedIndex = a;
                if (cdaSeries->valuename.at(a) == "eTot")
                    eTotIndex = a;
                if (cdaSeries->valuename.at(a) == "winCDA")
                    winCDAIndex = a;
                if (cdaSeries->valuename.at(a) == "rawCDA")
                    rawCDAIndex = a;
                if (cdaSeries->valuename.at(a) == "ekfAlt")
                    ekfAltIndex = a;
                if (cdaSeries->valuename.at(a) == "bcvAlt")
                    bcvAltIndex = a;
                if (cdaSeries->valuename.at(a) == "huAlt")
                    huAltIndex = a;
                if (cdaSeries->valuename.at(a) == "airdensity")
                    airDensityIndex = a;
                if (cdaSeries->valuename.at(a) == "airpressure")
                    airpressureIndex = a;
                if (cdaSeries->valuename.at(a) == "wind")
                    windIndex = a;
            }
            return 1;
        }
        else {
            cout << "No CDA data" << endl;
            return 0;
        }
    }
    else {
        return 0;
    }
}

int 
NotioCDAData::getNumPoints()
{
    //cout << "NotioCDA getNumPoints" << endl;
#ifdef MARC
    if ( cdaSeries != NULL )
        return cdaSeries->datapoints.count();
    else
        return 0;
#else
    return ride->dataPoints().count();
#endif
}

double 
NotioCDAData::getSecs(int i)
{
#ifdef MARC
    if ( cdaSeries != NULL ) {
        XDataPoint* xpoint = cdaSeries->datapoints.at(i);
        if ( xpoint != NULL ) {
            return xpoint->secs;
        }
        else {
            cout << "getSec Null at " << i;
            return 0.0;
        }
    }
    else
        return 0;
#else
    return ride->dataPoints().at(i)->secs;
#endif
}

double 
NotioCDAData::getCorrectedAlt(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[ekfAltIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioCDAData::getEkfCDA(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[rawCDAIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioCDAData::getOriginalAlt(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[bcvAltIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioCDAData::getHeadUnitAlt(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[huAltIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioCDAData::getWind(int i, const int iOrder)
{
    double sum = 0.0;
    if (cdaSeries)
    {
        int wActualOrder = static_cast<int>(iOrder / NotioFuncCompute::estimateRecInterval(cdaSeries));
        if ((i >= wActualOrder) && (i < (cdaSeries->datapoints.count() - wActualOrder)))
        {
            for (int j = i - wActualOrder; j <= i + wActualOrder ; j++) {
                sum += cdaSeries->datapoints.at(j)->number[windIndex];
            }
            sum /= ((wActualOrder * 2) + 1);
        }
    }
    return sum * 3.6;
}

double 
NotioCDAData::getVirtualWind(int i, double riderFactor, double riderExponent, const int iOrder)
{
    double sum = 0.0;

    if (cdaSeries)
    {
        int wActualOrder = static_cast<int>(iOrder / NotioFuncCompute::estimateRecInterval(cdaSeries));
        if ((i >= wActualOrder) && (i < (cdaSeries->datapoints.count() - wActualOrder)))
        {
            for (int j = i - wActualOrder; j <= i + wActualOrder; j++)
            {
                XDataPoint *xpoint = cdaSeries->datapoints.at(j);
                double windSpeed = 0;
                double airPressure = xpoint->number[airpressureIndex] / AeroAlgo::cAirPressureSensorFactor;
                if (airPressure > 0 )
                {
                    double airdensity = xpoint->number[airDensityIndex];
                    double airSpeed = AeroAlgo::calculateHeadwind(riderFactor, riderExponent, airPressure, airdensity);

                    windSpeed = airSpeed - (xpoint->number[speedIndex] * Utility::cMeterPerSecToKph);
                }

                sum += windSpeed;
            }
            sum /= ((wActualOrder * 2) + 1);
        }
    }
    return sum;
}

double
NotioCDAData::getDP(int i)
{
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    double DP = xpoint->number[airpressureIndex] / AeroAlgo::cAirPressureSensorFactor;

    return DP;
}

double
NotioCDAData::getWatts(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[eTotIndex];
#else
    return ride->dataPoints().at(i)->watts;
#endif
}

double
NotioCDAData::getKPH(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[speedIndex] * 3.6;
#else
    return ride->dataPoints().at(i)->kph;
#endif
}
double 
NotioCDAData::getHeadWind(int i)
{
    return thisRide->dataPoints().at(i)->headwind;
}

double 
NotioCDAData::getKM(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->km/1000.0;
#else
    return ride->dataPoints().at(i)->km;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAData::intervalCDA
///        This method calculates the CdA for a specific interval.
///
/// \param[in] iStart   Interval start index.
/// \param[in] iEnd     Interval end index.
///
/// \return The CdA value for the interval.
///////////////////////////////////////////////////////////////////////////////
double NotioCDAData::intervalCDA(int iStart, int iEnd)
{
    double wCda = 0.0;

    if (thisRide && cdaSeries)
    {
        int wETotIndex = cdaSeries->valuename.indexOf("eTot");
        int wECrrIndex = cdaSeries->valuename.indexOf("eCrr");
        int wEAccIndex = cdaSeries->valuename.indexOf("eAcc");
        int wEDeHIndex = cdaSeries->valuename.indexOf("eDeH");
        int wEAirIndex = cdaSeries->valuename.indexOf("eAir");
        int wAirPressureIndex = cdaSeries->valuename.indexOf("airpressure");

        if ((wETotIndex > -1) && (wECrrIndex > -1) && (wEAccIndex > -1) &&
                (wEDeHIndex > -1) && (wEAirIndex > -1) && (wAirPressureIndex > -1))
        {
            double wJoules_pm_total = 0.0;
            double wJoules_crr_total = 0.0;
            double wJoules_cda_total = 0.0;
            double wJoules_alt_total = 0.0;
            double wJoules_inertia_total = 0.0;

            for (int i = iStart; (i < iEnd) && (i < cdaSeries->datapoints.count()); i++)
            {
                XDataPoint *xpoint = cdaSeries->datapoints[i];
                double wAirPressure = xpoint->number[wAirPressureIndex] / AeroAlgo::cAirPressureSensorFactor;

                wJoules_pm_total += (xpoint->number[wETotIndex] * m_MechEff);
                wJoules_crr_total += AeroAlgo::rolling_resistance_energy(m_Crr, m_TotalWeight, xpoint->number[wECrrIndex]);
                wJoules_alt_total += AeroAlgo::potential_energy(m_TotalWeight, xpoint->number[wEDeHIndex]);
                wJoules_inertia_total += AeroAlgo::kinetic_energy(m_TotalWeight, m_InertiaFactor, xpoint->number[wEAccIndex]);
                wJoules_cda_total += AeroAlgo::aero_energy(m_RiderFactor, m_RiderExponent, wAirPressure, xpoint->number[wEAirIndex]);
            }

            wCda = AeroAlgo::calculate_cda(wJoules_cda_total, wJoules_pm_total, wJoules_alt_total, wJoules_inertia_total, wJoules_crr_total);
        }
    }
    return wCda;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAData::intervalCDA
///        This method calculates the average CdA for a specific interval.
///
/// \param[in] iInterval    Interval.
/// \return The average CdA value.
///////////////////////////////////////////////////////////////////////////////
double NotioCDAData::intervalCDA(IntervalItem *iInterval)
{
    if (cdaSeries && iInterval)
    {
        int wStartIndex = cdaSeries->timeIndex(iInterval->start);
        int wStopIndex = cdaSeries->timeIndex(iInterval->stop);
        return intervalCDA(wStartIndex, wStopIndex);
    }
    else
        return 0;
}
