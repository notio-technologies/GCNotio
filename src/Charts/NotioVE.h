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

#ifndef _GC_NotioVE_h
#define _GC_NotioVE_h 1
#include "GoldenCheetah.h"

#include <qwt_plot.h>
#include <qwt_series_data.h>
#include <qwt_plot_canvas.h>
#include <QtGui>

#include <QWidget>
#include <QFrame>
#include <QTableWidget>
#include <QTextEdit>
#include <QStackedWidget>

#include "LTMWindow.h" // for tooltip/canvaspicker

// forward references
class NotioVEWindow;
class IntervalNotioVEData;

#include "NotioData.h"

class NotioVEData  {
public :
    NotioVEData();
    int setData(RideFile *ride);

    int eTotIndex = -1;
    int speedIndex = -1;
    int ekfAltIndex = -1;
    int bcvAltIndex = -1;
    int huAltIndex = -1;
    int airpressureIndex = -1;
    int airDensityIndex = -1;
    int windIndex = -1;
    XDataSeries *cdaSeries;
    int dataPoints;

    int getNumPoints();
    double getSecs(int);
    double getWind(int);
    double getVirtualWind(int,double,double);
    double getCorrectedAlt(int);
    double getOriginalAlt(int);
    double getHeadUnitAlt(int);
    double getWatts(int);
    double getDP(int);
    double getKPH(int);
    double getHeadWind(int);
    double getKM(int);
    double slope(double, double, double, double, double, double,double, int, double);

    RideFile *ride;
};

class NotioVE : public QwtPlot {

    Q_OBJECT
    G_OBJECT


    public:
        NotioVE( NotioVEWindow *, Context * );
    bool byDistance() const { return bydist; }
    bool useMetricUnits;  // whether metric units are used (or imperial)
    void setData(RideItem *_rideItem, bool new_zoom);
    void setAxisTitle(int axis, QString label);

    void refreshIntervalMarkers();

private:

    NotioVEData NotioVEData;
    bool GarminON;
    bool WindOn;
    Context *context;
    NotioVEWindow *parent;

    LTMToolTip      *tooltip;
    LTMCanvasPicker *_canvasPicker; // allow point selection/hover

    void adjustEoffset();

public slots:

    void setAutoEoffset(int value);
    void setConstantAlt(int value);
    void setGarminON(int value);
    void setWindOn(int value);
    void setByDistance(int value);
    void configChanged(qint32);

    void pointHover( QwtPlotCurve *, int );

signals:

protected:
    friend class ::NotioVEWindow;
    friend class ::IntervalNotioVEData;


    QwtPlotGrid *grid;
    QVector<QwtPlotMarker*> d_mrk;

    // One curve to plot in the Course Profile:
    QwtPlotCurve *windCurve;   // virtual elevation curve
    QwtPlotCurve *vWindCurve;   // virtual elevation curve
    QwtPlotCurve *vZeroCurve;   // virtual elevation curve
    QwtPlotCurve *veCurve;   // virtual elevation curve
    QwtPlotCurve *altCurve;    // Corrected elevation curve
    QwtPlotCurve *altInCurve;    // Original altitude curve
    QwtPlotCurve *altHeadUnitCurve;    // Garmin, Head Unit curve

    QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot

    RideItem *rideItem;

    QVector<double> hrArray;
    QVector<double> wattsArray;
    QVector<double> speedArray;
    QVector<double> cadArray;

    // We store virtual elevation, time, altitude,and distance:
    QVector<double> windArray;
    QVector<double> vWindArray;
    QVector<double> vZeroArray;
    QVector<double> syncArray;
    QVector<double> veArray;
    QVector<double> altArray;
    QVector<double> altInArray;
    QVector<double> altHeadUnitArray;
    QVector<double> timeArray;
    QVector<double> distanceArray;

    int smooth;
    bool bydist;
    bool autoEoffset;
    bool constantAlt;
    int arrayLength;
    int iCrr;
    int iCda;
    double crr;
    double cda;
    double totalMass; // Bike + Rider mass
    double riderFactor;
    double riderExponent;
    double eta;
    double wheelMomentInertia;
    double eoffset;


    //double   slope(double, double, double, double, double, double, double);
    void     recalc(bool);
    void     setYMax(bool);
    void     setXTitle();
    void     setIntCrr(int);
    void     setIntCda(int);
    void     setIntRiderFactor(int);
    void     setIntRiderExponent(int);
    void     setIntEta(int);
    void     setIntEoffset(int);
    void     setIntTotalMass(int);
    double   getCrr() const { return (double)crr; }
    double   getCda() const { return (double)cda; }
    double   getTotalMass() const { return (double)totalMass; }
    double   getRiderFactor() const { return (double)riderFactor; }
    double   getRiderExponent() const { return (double)riderExponent; }
    double   getEta() const { return (double)eta; }
    double   getEoffset() const { return (double)eoffset; }
    int      intCrr() const { return (int)( crr * 1000000  ); }
    int      intCda() const { return (int)( cda * 10000); }
    int      intTotalMass() const { return (int)( totalMass * 100); }
    int      intRiderFactor() const { return (int)( riderFactor * 10000); }
    int      intRiderExponent() const { return (int)( riderExponent * 10000); }
    int      intEta() const { return (int)( eta * 10000); }
    int      intEoffset() const { return (int)( eoffset * 100); }
    QString  estimateCdACrr(RideItem* rideItem);

};

#endif // _GC_NotioVE_h

