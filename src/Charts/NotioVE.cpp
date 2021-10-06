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

#include "NotioVE.h"
#include "NotioVEWindow.h"
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

#define PI M_PI

#define MARC
static inline double
max(double a, double b) { if (a > b) return a; else return b; }
static inline double
min(double a, double b) { if (a < b) return a; else return b; }

bool
isAlt(const RideFileDataPresent *dataPresent)
{
    return true;
}
bool
isHeadWind(const RideFileDataPresent *dataPresent)
{
    return true;
}
bool
isWatts(const RideFileDataPresent *dataPresent)
{
    return true;
}

/*----------------------------------------------------------------------
 * Interval plotting
 *--------------------------------------------------------------------*/

class IntervalNotioVEData : public QwtSeriesData<QPointF>
{
public:
    NotioVE *aerolab;
    Context *context;
    IntervalNotioVEData(NotioVE *aerolab, Context *context) : aerolab( aerolab ), context(context) { }

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
IntervalItem *IntervalNotioVEData::intervalNum(int number) const
{
    if (number >= 0 && aerolab->rideItem->intervalsSelected().count() > number)
        return aerolab->rideItem->intervalsSelected().at(number);

    return NULL;
}

// ------------------------------------------------------------------------------------------------------------
// how many intervals selected?
// ------------------------------------------------------------------------------------------------------------
int IntervalNotioVEData::intervalCount() const
{
    return aerolab->rideItem->intervalsSelected().count();
}
/*
 * INTERVAL HIGHLIGHTING CURVE
 * IntervalNotioVEData - implements the qwtdata interface where
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
double IntervalNotioVEData::x(size_t number) const
{
    // for each interval there are four points, which interval is this for?
    // interval numbers start at 1 not ZERO in the utility functions

    double result = 0;

    int interval_no = number ? 1 + number / 4 : 1;
    // get the interval
    IntervalItem *current = intervalNum( interval_no );

    if ( current != NULL )
    {
        double multiplier = context->athlete->useMetricUnits ? 1 : MILES_PER_KM;
        // which point are we returning?
        //qDebug() << "number = " << number << endl;
        switch ( number % 4 )
        {
        case 0 : result = aerolab->bydist ? multiplier * current->startKM : current->start/60; // bottom left
            break;
        case 1 : result = aerolab->bydist ? multiplier * current->startKM : current->start/60; // top left
            break;
        case 2 : result = aerolab->bydist ? multiplier * current->stopKM  : current->stop/60;  // bottom right
            break;
        case 3 : result = aerolab->bydist ? multiplier * current->stopKM  : current->stop/60;  // top right
            break;
        }
    }
    return result;
}
double IntervalNotioVEData::y(size_t number) const
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

size_t IntervalNotioVEData::size() const
{
    return intervalCount() * 4;
}

QPointF IntervalNotioVEData::sample(size_t i) const {
    return QPointF(x(i), y(i));
}

QRectF IntervalNotioVEData::boundingRect() const
{
    return QRectF(-5000, 5000, 10000, 10000);
}

//**********************************************
//**        END IntervalNotioVEData           **
//**********************************************


NotioVE::NotioVE(NotioVEWindow *parent, Context *context) :
    QwtPlot(parent), context(context), parent(parent), rideItem(NULL),
    smooth(1), bydist(true), autoEoffset(false)
{
    crr       = 0.005;
    cda       = 0.500;
    totalMass = 85.0;
    riderFactor       = 1.234;
    riderExponent     = 4.321;
    eta       = 1.0;
    wheelMomentInertia       = 1.0;
    eoffset   = 0.0; // In constructor
    constantAlt = false;

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);

    setXTitle();
    setAxisTitle(yLeft, tr("Elevation (m)"));
    setAxisScale(yLeft, -300, 300);

    setAxisTitle(yRight, tr("Wind"));
    setAxisScale(yRight, -10, 10);

    setAxisVisible(yRight, true);

    setAxisTitle(xBottom, tr("Distance (km)"));
    setAxisScale(xBottom, 0, 60);

    veCurve = new QwtPlotCurve(tr("V-Elevation"));
    windCurve = new QwtPlotCurve(tr("Wind"));
    vWindCurve = new QwtPlotCurve(tr("vWind"));
    vZeroCurve = new QwtPlotCurve(tr("vWind"));
    altCurve = new QwtPlotCurve(tr("Corrected Elevation"));
    altInCurve = new QwtPlotCurve(tr("Elevation"));
    altHeadUnitCurve = new QwtPlotCurve(tr("HU Elevation"));

    // get rid of nasty blank space on right of the plot
    veCurve->setYAxis( yLeft );
    windCurve->setYAxis( yRight );
    vWindCurve->setYAxis( yRight );
    vZeroCurve->setYAxis( yRight );
    altCurve->setYAxis( yLeft );
    altInCurve->setYAxis( yLeft );
    altHeadUnitCurve->setYAxis( yLeft );

    intervalHighlighterCurve = new QwtPlotCurve();
    intervalHighlighterCurve->setBaseline(-5000);
    intervalHighlighterCurve->setYAxis( yLeft );
    intervalHighlighterCurve->setSamples( new IntervalNotioVEData( this, context ) );
    intervalHighlighterCurve->attach( this );
    //XXX broken this->legend()->remove( intervalHighlighterCurve ); // don't show in legend

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    configChanged(CONFIG_APPEARANCE);
}


void
NotioVE::configChanged(qint32)
{

    // set colors
    setCanvasBackground(GColor(CPLOTBACKGROUND));

    QPen vePen = QPen(GColor(CAEROVE));
    vePen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    veCurve->setPen(vePen);

    QPen windPen = QPen(GColor(CPLOTMARKER));
    windPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    windCurve->setPen(windPen);

    QPen vWindPen = QPen(GColor(CAEROVE));
    vWindPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    vWindCurve->setPen(vWindPen);

    QPen vZeroPen = QPen(GColor(CAEROVE));
    vZeroPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    vZeroCurve->setPen(vZeroPen);

    QPen altPen = QPen(GColor(CAEROEL));
    altPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    altCurve->setPen(altPen);

    QPen altInPen = QPen(GColor(CPOWER));
    altInPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    altInCurve->setPen(altInPen);

    QPen altHeadUnitPen = QPen(GColor(CALTITUDE));
    altHeadUnitPen.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());
    altHeadUnitCurve->setPen(altHeadUnitPen);

    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);

    QPen ihlPen = QPen( GColor( CINTERVALHIGHLIGHTER ) );
    ihlPen.setWidth(1);
    intervalHighlighterCurve->setPen( ihlPen );

    QColor ihlbrush = QColor(GColor(CINTERVALHIGHLIGHTER));
    ihlbrush.setAlpha(40);
    intervalHighlighterCurve->setBrush(ihlbrush);   // fill below the line

    //XXX broken this->legend()->remove( intervalHighlighterCurve ); // don't show in legend
    QPalette palette;
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    axisWidget(QwtPlot::yLeft)->setPalette(palette);

    axisWidget(QwtPlot::yRight)->setPalette(palette);
}

static int supercount = 0;

void
NotioVE::setData(RideItem *_rideItem, bool new_zoom) {

    // HARD-CODED DATA: p1->kph
    double vfactor = 3.600;
    double m       =  totalMass;
    double small_number = 0.00001;

    rideItem = _rideItem;
    RideFile *ride = rideItem->ride();

    if ( NotioVEData.setData(ride) == 0 )
        return ;

    veArray.clear();
    windArray.clear();
    vWindArray.clear();
    vZeroArray.clear();
    syncArray.clear();
    altArray.clear();
    altInArray.clear();
    altHeadUnitArray.clear();
    distanceArray.clear();
    timeArray.clear();

    if( ride ) {

        const RideFileDataPresent *dataPresent = ride->areDataPresent();
        //setTitle(ride->startTime().toString(GC_DATETIME_FORMAT));

        if( isWatts(dataPresent) ) {

            // If watts are present, then we can fill the veArray data:
            const RideFileDataPresent *dataPresent = ride->areDataPresent();
            int npoints = NotioVEData.getNumPoints(); //ride->dataPoints().size();
            double dt = 1.0;//ride->recIntSecs(); //CDAData is in seconds and recInt looks at Samples, not XDATA
            veArray.resize(isWatts(dataPresent) ? npoints : 0);
            windArray.resize(isWatts(dataPresent) ? npoints : 0);
            vWindArray.resize(isWatts(dataPresent) ? npoints : 0);
            vZeroArray.resize(isWatts(dataPresent) ? npoints : 0);
            syncArray.resize(isWatts(dataPresent) ? npoints : 0);
            altArray.resize(isAlt(dataPresent) || constantAlt ? npoints : 0);
            altInArray.resize(isAlt(dataPresent) || constantAlt ? npoints : 0);
            altHeadUnitArray.resize(isAlt(dataPresent) || constantAlt ? npoints : 0);
            timeArray.resize(isWatts(dataPresent) ? npoints : 0);
            distanceArray.resize(isWatts(dataPresent) ? npoints : 0);

            {
                int dividor = (int)(1.0/ride->recIntSecs());
                for ( int i = 0 ; i < npoints ; i++ )
                    syncArray[i] = 0.0;

                foreach(IntervalItem *interval, rideItem->intervals()) {
                    //        	    	if (interval->type != RideFileInterval::USER) continue;
                    if (interval->selected) {
                        qDebug() << "Interval selected" << interval->name ;
                        int intervalStart = interval->start;
                        qDebug() << "Interval start" << intervalStart;
                        int idx2 = rideItem->ride()->timeIndex(intervalStart);
                        syncArray[idx2/dividor] = 1.0;
                    }
                }
            }

            // quickly erase old data

            veCurve->setVisible(false);
            windCurve->setVisible(false);
            vWindCurve->setVisible(false);
            vZeroCurve->setVisible(false);

            altCurve->setVisible(false);
            altInCurve->setVisible(false);
            altHeadUnitCurve->setVisible(false);


            veCurve->detach();
            if (!veArray.empty()) {
                veCurve->attach(this);
                veCurve->setVisible(isWatts(dataPresent));
            }

            // detach and re-attach the ve curve:
            windCurve->detach();
            if (!windArray.empty()) {
                windCurve->attach(this);
                if ( WindOn == true )
                    windCurve->setVisible(isWatts(dataPresent));
            }

            // detach and re-attach the ve curve:
            vWindCurve->detach();
            if (!vWindArray.empty()) {
                vWindCurve->attach(this);
                if ( WindOn == true )
                    vWindCurve->setVisible(isWatts(dataPresent));
            }

            // detach and re-attach the ve curve:
            vZeroCurve->detach();
            if (!vZeroArray.empty()) {
                vZeroCurve->attach(this);
                if ( WindOn == true )
                    vZeroCurve->setVisible(isWatts(dataPresent));
            }

            altInCurve->detach();
            if (!altInArray.empty()) {
                altInCurve->attach(this);
                if ( GarminON == true )
                    altInCurve->setVisible(isWatts(dataPresent));
            }
            altHeadUnitCurve->detach();
            if (!altHeadUnitArray.empty()) {
                altHeadUnitCurve->attach(this);
                if ( GarminON == true )
                    altHeadUnitCurve->setVisible(isWatts(dataPresent));
            }

            // detach and re-attach the ve curve:
            bool have_recorded_alt_curve = false;
            altCurve->detach();
            if (!altArray.empty()) {
                have_recorded_alt_curve = true;
                altCurve->attach(this);
                altCurve->setVisible(isAlt(dataPresent) || constantAlt );
            }

            // Fill the virtual elevation profile with data from the ride data:
            double t = 0.0;
            double vlast = 0.0;
            double e     = 0.0;
            arrayLength = 0;

            int _numPoints =NotioVEData.getNumPoints();


            //FILE *veLog = fopen("/Users/marcg/Dropbox/temp/veLog.csv","w");

            for( int p1Index = 0 ; p1Index < _numPoints ; p1Index++ ) {
                // If this is first element, set e to eoffser
                if ( arrayLength == 0 )
                    e = eoffset;

                timeArray[arrayLength]  = NotioVEData.getSecs(p1Index) / 60.0;

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
                            altArray[arrayLength] = (context->athlete->useMetricUnits
                                                     ? NotioVEData.getCorrectedAlt(p1Index)
                                                     : NotioVEData.getCorrectedAlt(p1Index) * FEET_PER_METER);
                            altInArray[arrayLength] = (context->athlete->useMetricUnits
                                                       ? NotioVEData.getOriginalAlt(p1Index)
                                                       : NotioVEData.getOriginalAlt(p1Index) * FEET_PER_METER);
                            altHeadUnitArray[arrayLength] = (context->athlete->useMetricUnits
                                                             ? NotioVEData.getHeadUnitAlt(p1Index)
                                                             : NotioVEData.getHeadUnitAlt(p1Index) * FEET_PER_METER);
                        }
                    }
                }
                else {
                    cout << "P1Index " << p1Index << endl;
                }

                // Unpack:
                distanceArray[arrayLength] = NotioVEData.getKM(p1Index);
                double power = max(0.0, NotioVEData.getWatts(p1Index));
                double v     = NotioVEData.getKPH(p1Index)/vfactor;

                double f     = 0.0;
                double a     = 0.0;

                wheelMomentInertia = 1.0;

                if( v > small_number ) {
                    f  = power/v;
                    a  = ( v*v - vlast*vlast ) / ( 2.0 * dt * v ) * wheelMomentInertia;
                } else {
                    a = ( v - vlast ) / dt * wheelMomentInertia;
                }

                f *= eta; // adjust for drivetrain efficiency if using a crank-based meter
                double s   = NotioVEData.slope( f, a, m, crr, cda, riderFactor, riderExponent,p1Index,eta );
                double de  = s * v * dt * (context->athlete->useMetricUnits ? 1 : FEET_PER_METER);
                e += de;
                t += dt;


                //fprintf(veLog,"%4d,%6.4f,%6.4f,%6.2f,%6.4f,%6.4f,%4.2f,%8.4f\n",p1Index,f,a,m,s,v,dt,de);

                veArray[arrayLength] = e;

                windArray[arrayLength] = NotioVEData.getWind(p1Index);
                vWindArray[arrayLength] = NotioVEData.getVirtualWind(p1Index,riderFactor,riderExponent);
                vZeroArray[arrayLength] = 0.0;

                if (  syncArray[arrayLength] == 1.0 ) {
                    e = altArray[arrayLength];
                }

                vlast = v;

                ++arrayLength;
            }
            //fclose(veLog);
        } else {
            veCurve->setVisible(false);
            windCurve->setVisible(false);
            vWindCurve->setVisible(false);
            vZeroCurve->setVisible(false);
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
NotioVE::setAxisTitle(int axis, QString label)
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

void
NotioVE::adjustEoffset() {

    if (autoEoffset && !altArray.empty()) {

        int dividor = (int)(1.0/rideItem->ride()->recIntSecs());

        double idx = axisScaleDiv( QwtPlot::xBottom ).lowerBound();
        parent->eoffsetSlider->setEnabled(false);

        //cout << "cowabunga idx" << idx << endl;

        //CDAData is in seconds and recInt looks at Samples, not XDATA

        if (bydist) {
            //cout << "cowabunga bydist idx" << idx << endl;
            int idx2 = rideItem->ride()->distanceIndex(idx);
            //cout << "cowabunga bydist  idx2" << idx2 << endl;

            int v = 100*(altArray.at(idx2/dividor)-veArray.at(idx2/dividor));
            parent->eoffsetSlider->setValue(intEoffset()+v);
        }
        else {
            //cout << "cowabunga bytime idx" << idx << endl;
            int idx2 = rideItem->ride()->timeIndex(60*idx);
            //cout << "cowabunga bytime  idx2" << idx2 << endl;

            int v = 100*(altArray.at(idx2/dividor)-veArray.at(idx2/dividor));
            parent->eoffsetSlider->setValue(intEoffset()+v);
        }
    }
    else {
        parent->eoffsetSlider->setEnabled(true);
    }
    //cout << "cowabunga done" << endl;
}


struct DataPoint {
    double time, hr, watts, speed, cad, alt;
    DataPoint(double t, double h, double w, double s, double c, double a) :
        time(t), hr(h), watts(w), speed(s), cad(c), alt(a) {}
};

void
NotioVE::recalc( bool new_zoom ) {

    if (timeArray.empty())
        return;

    int rideTimeSecs = (int) ceil(timeArray[arrayLength - 1]);
    int totalRideDistance = (int ) ceil(distanceArray[arrayLength - 1]);

    // If the ride is really long, then avoid it like the plague.
    if (rideTimeSecs > SECONDS_IN_A_WEEK) {
        QVector<double> data;

        if (!veArray.empty()){
            veCurve->setSamples(data, data);
        }
        if (!windArray.empty()){
            windCurve->setSamples(data, data);
        }
        if (!vWindArray.empty()){
            vWindCurve->setSamples(data, data);
        }
        if (!vZeroArray.empty()){
            vZeroCurve->setSamples(data, data);
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

    QVector<double> &xaxis = (bydist?distanceArray:timeArray);
    int startingIndex = 0;
    int totalPoints   = arrayLength - startingIndex;

    // set curves

    if (!veArray.empty()) {
        veCurve->setSamples(xaxis.data() + startingIndex, veArray.data() + startingIndex, totalPoints);
    }
    if (!windArray.empty()) {
        windCurve->setSamples(xaxis.data() + startingIndex, windArray.data() + startingIndex, totalPoints);
    }
    if (!vWindArray.empty()) {
        vWindCurve->setSamples(xaxis.data() + startingIndex, vWindArray.data() + startingIndex, totalPoints);
    }
    if (!vZeroArray.empty()) {
        vZeroCurve->setSamples(xaxis.data() + startingIndex, vZeroArray.data() + startingIndex, totalPoints);
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
        setAxisScale(xBottom, 0.0, (bydist?totalRideDistance:rideTimeSecs));

    setYMax(new_zoom );
    refreshIntervalMarkers();

    replot();
}

void
NotioVE::setYMax(bool new_zoom)
{
    if (veCurve->isVisible())
    {
        if ( context->athlete->useMetricUnits )
        {
            setAxisTitle( yLeft, tr("Elevation (m)") );
            setAxisTitle( yRight, tr("Wind") );
        }

        else
        {
            setAxisTitle( yLeft, tr("Elevation (')") );
            setAxisTitle( yRight, tr("Wind") );
        }

        double minY = 0.0;
        double maxY = 0.0;

        //************

        //if (veCurve->isVisible()) {
        // setAxisTitle(yLeft, tr("Elevation"));
        if ( !altArray.empty() ) {
            //   setAxisScale(yLeft,
            //          min( veCurve->minYValue(), altCurve->minYValue() ) - 10,
            //          10.0 + max( veCurve->maxYValue(), altCurve->maxYValue() ) );

            //mg set Xais min/max
            minY = min( veCurve->minYValue(), altCurve->minYValue() ) - 3;//10;
            maxY = 3.0 + max( veCurve->maxYValue(), altCurve->maxYValue() );

        } else {
            //setAxisScale(yLeft,
            //       veCurve->minYValue() ,
            //       1.05 * veCurve->maxYValue() );

            if ( new_zoom )

            {
                minY = veCurve->minYValue();
                maxY = veCurve->maxYValue();
            }
            else
            {
                minY = parent->getCanvasTop();
                maxY = parent->getCanvasBottom();
            }

            //adjust eooffset
            // TODO
        }

        setAxisScale( yLeft, minY, maxY );
        setAxisScale( yRight, -10.0, 10.0 );
        setAxisLabelRotation(yLeft,270);
        setAxisLabelRotation(yRight,270);
        setAxisLabelAlignment(yLeft,Qt::AlignVCenter);
        setAxisLabelAlignment(yRight,Qt::AlignVCenter);
    }

    enableAxis(yLeft, veCurve->isVisible());
}

void
NotioVE::setXTitle() {

    if (bydist)
        setAxisTitle(xBottom, tr("Distance ")+QString(context->athlete->useMetricUnits?"(km)":"(miles)"));
    else
        setAxisTitle(xBottom, tr("Time (minutes)"));
}

void
NotioVE::setAutoEoffset(int value)
{
    autoEoffset = value;
    cout << "set auto Adjust" << endl;
    adjustEoffset();
    cout << "Adjust done" << endl;
}

void
NotioVE::setConstantAlt(int value)
{
    constantAlt = value;
}
void
NotioVE::setGarminON(int value)
{
    GarminON = (bool)value;
}

void
NotioVE::setWindOn(int value)
{
    WindOn = (bool)value;
}

void
NotioVE::setByDistance(int value) {
    bydist = value;
    setXTitle();
    recalc(true);
}


// At slider 1000, we want to get max Crr=0.1000
// At slider 1    , we want to get min Crr=0.0001
void
NotioVE::setIntCrr(int value) {

    crr = (double) value / 1000000.0;

    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioVE::setIntCda(int value)  {
    cda = (double) value / 10000.0;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioVE::setIntTotalMass(int value) {

    totalMass = (double) value / 100.0;
    recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioVE::setIntRiderFactor(int value) {

    riderFactor = (double) value / 10000.0;
    cout << "Set Int RiderFactor " << riderFactor << endl;
    recalc(false);
}

// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioVE::setIntRiderExponent(int value) {

    riderExponent = (double) value / 10000.0;
    cout << "Set Int RiderExponent " << riderExponent << endl;
    recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioVE::setIntEta(int value) {

    eta = (double) value / 10000.0;
    //eta = 0.98;
    //wheelMomentInertia = (double) value / 10000.0;
    recalc(false);
}


// At slider 1000, we want to get max CdA=1.000
// At slider 1    , we want to get min CdA=0.001
void
NotioVE::setIntEoffset(int value) {

    eoffset = (double) value / 100.0;
    recalc(false);
}

void NotioVE::pointHover (QwtPlotCurve *curve, int index)
{
    if ( index >= 0 && curve != intervalHighlighterCurve )
    {
        double x_value = curve->sample( index ).x();
        double y_value = curve->sample( index ).y();
        // output the tooltip

        QString text = QString( "%1 %2 %3 %4 %5" )
                . arg( this->axisTitle( curve->xAxis() ).text() )
                . arg( x_value, 0, 'f', 3 )
                . arg( "\n" )
                . arg( this->axisTitle( curve->yAxis() ).text() )
                . arg( y_value, 0, 'f', 3 );

        // set that text up
        tooltip->setText( text );
    }
    else
    {
        // no point
        tooltip->setText( "" );
    }
}

void NotioVE::refreshIntervalMarkers()
{
    cout << "Refresh Interval Markers" << endl;

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
            if (interval->type != RideFileInterval::USER) continue;

            QwtPlotMarker *mrk = new QwtPlotMarker;
            d_mrk.append(mrk);
            mrk->attach(this);
            mrk->setLineStyle(QwtPlotMarker::VLine);
            mrk->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

            if ( interval->selected )
                mrk->setLinePen(QPen(GColor(CAEROEL), 0, Qt::DashDotLine));
            else
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
                mrk->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                              interval->startKM, 0.0);
            }
            mrk->setLabel(text);

            if ( interval->selected ) {
                QwtPlotMarker *mrkEnd = new QwtPlotMarker;
                d_mrk.append(mrkEnd);
                mrkEnd->attach(this);
                mrkEnd->setLineStyle(QwtPlotMarker::VLine);
                mrkEnd->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
                mrkEnd->setLinePen(QPen(GColor(CAEROVE), 0, Qt::DashDotLine));
                QwtText text(interval->name);
                text.setFont(QFont("Helvetica", 10, QFont::Bold));
                text.setColor(GColor(CPLOTMARKER));
                if (!bydist) {
                    //cout << "Interval Start time" << interval->start << endl;
                    mrkEnd->setValue(interval->stop / 60.0, 0.0);
                }
                else {
                    //cout << "Interval Start km " << interval->startKM << endl;
                    mrkEnd->setValue((context->athlete->useMetricUnits ? 1 : MILES_PER_KM) *
                                     interval->stopKM, 0.0);
                }
                mrkEnd->setLabel(text);
            }
        }
    }
}

int
setupEstimateAltitudes(RideItem *rideItem)
{
    RideFile *ride = rideItem->ride();
    int dividor = (int)(1.0/ride->recIntSecs());
    foreach(IntervalItem *interval, rideItem->intervals()) {
        if (interval->selected) {
            //cout << "Interval selected" << interval->name ;
            int intervalStart = interval->start;
            cout << "Interval start" << intervalStart << endl;
            int intervalEnd = interval->stop;
            cout << "Interval end" << intervalEnd << endl;
            int idx2 = rideItem->ride()->timeIndex(intervalStart);
            //syncArray[idx2/dividor] = 1.0;
        }
    }
    return 1;
}
/*
 * Estimate CdA and Crr usign energy balance in segments defined by
 * non-zero altitude.
 * Returns an explanatory error message ff it fails to do the estimation,
 * otherwise it updates cda and crr and returns an empty error message.
 * Author: Alejandro Martinez
 * Date: 23-aug-2012
 */
QString NotioVE::estimateCdACrr(RideItem *rideItem)
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
            int _numPoints = NotioVEData.getNumPoints();
            for( int p1Index = 0 ; p1Index < _numPoints ; p1Index++ ) {
                // Unpack:
                double power = max(0.0, NotioVEData.getWatts(p1Index));
                double v     = NotioVEData.getKPH(p1Index)/vfactor;
                double distance = v * dt;
#ifdef MARC
                double DP = NotioVEData.getDP(p1Index);
#else
                double headwind = v;
                if( isHeadWind(dataPresent) ) {
                    headwind   = p1->headwind/vfactor;
                }
#endif
                double alt = NotioVEData.getCorrectedAlt(p1Index);
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
                    // round and update if the values are in NotioVE's range
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

NotioVEData::NotioVEData()
{
    ekfAltIndex = -1;
    bcvAltIndex = -1;
    huAltIndex = -1;
    windIndex = -1;
    airpressureIndex = -1;
    airDensityIndex = -1;
}

int
NotioVEData::setData(RideFile *inRide)
{
    ride = inRide;
    if ( ride != NULL ) {
        cdaSeries  = ride->xdata("CDAData"); //MG
        if ( cdaSeries != NULL ) {            
            speedIndex = cdaSeries->valuename.indexOf("speed");
            eTotIndex = cdaSeries->valuename.indexOf("eTot");
            ekfAltIndex = cdaSeries->valuename.indexOf("ekfAlt");
            bcvAltIndex = cdaSeries->valuename.indexOf("bcvAlt");
            huAltIndex = cdaSeries->valuename.indexOf("huAlt");
            airDensityIndex = cdaSeries->valuename.indexOf("airdensity");
            airpressureIndex = cdaSeries->valuename.indexOf("airpressure");
            windIndex = cdaSeries->valuename.indexOf("wind");
            return 1;
        }
        else {
            cout << "No CDA data" << endl;
            return 0;
        }
    }
    return 0;
}

int 
NotioVEData::getNumPoints()
{
    //cout << "NotioVE getNumPoints" << endl;
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
NotioVEData::getSecs(int i)
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
NotioVEData::getCorrectedAlt(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[ekfAltIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioVEData::getOriginalAlt(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[bcvAltIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioVEData::getHeadUnitAlt(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[huAltIndex];
#else
    return ride->dataPoints().at(i)->alt;
#endif
}
double 
NotioVEData::getWind(int i)
{
#ifdef MARC
    double sum = 0.0;
    XDataPoint* xpoint;
    if ( i > 28 ) {
        for ( int j = i-29 ; j <= i ; j++ ) {
            xpoint = cdaSeries->datapoints.at(j);
            sum += xpoint->number[windIndex];
        }
        sum /= 30.0;
    }
    return sum;
#else
    return ride->dataPoints().at(i)->alt;
#endif
}

double 
NotioVEData::getVirtualWind(int i,double riderFactor,double riderExponent)
{
    double sum = 0.0;
    XDataPoint* xpoint;
    if ( i > 28) {
        for ( int j = i-29 ; j <= i ; j++ ) {
            xpoint = cdaSeries->datapoints.at(j);
            double windSpeed = 0;
            double airPressure = xpoint->number[airpressureIndex] / GcAlgo::AeroAlgo::cAirPressureSensorFactor;

            double airdensity = xpoint->number[airDensityIndex];
            double airSpeed = GcAlgo::AeroAlgo::calculateHeadwind(riderFactor, riderExponent, airPressure, airdensity) / GcAlgo::Utility::cMeterPerSecToKph;

            windSpeed = airSpeed - (xpoint->number[speedIndex]);

            sum += windSpeed;
        }
        sum /= 30.0;
    }
    return sum;
}

double
NotioVEData::getDP(int i)
{
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    double DP = xpoint->number[airpressureIndex] / GcAlgo::AeroAlgo::cAirPressureSensorFactor;
    if ( DP  < 0.0 )
        DP = 0.0;
    else
        DP = DP * std::pow(GcAlgo::AeroAlgo::calculateWindFactor(1.39, -0.05, DP), 2.0);

    return DP;
}

double
NotioVEData::getWatts(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[eTotIndex];
#else
    return ride->dataPoints().at(i)->watts;
#endif
}

double
NotioVEData::getKPH(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->number[speedIndex]*3.6;
#else
    return ride->dataPoints().at(i)->kph;
#endif
}
double 
NotioVEData::getHeadWind(int i)
{
    return ride->dataPoints().at(i)->headwind;
}

double 
NotioVEData::getKM(int i)
{
#ifdef MARC
    XDataPoint* xpoint = cdaSeries->datapoints.at(i);
    return xpoint->km/1000.0;
#else
    return ride->dataPoints().at(i)->km;
#endif
}
double
NotioVEData::slope(double f, double a, double m,
                   double crr, double cda, double riderFactor,
                   double riderExponent, int p1Index, double eta)  {
#ifdef MARC
    double DP = getDP(p1Index);

    double g = KG_FORCE_PER_METER;

    // Small angle version of slope calculation:
    double s = f/(m*g) - crr - cda*DP/(m*g) - a/g;
#endif

    return s;
}
