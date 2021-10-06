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

#ifndef _GC_NotioCDA_h
#define _GC_NotioCDA_h 1
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
class NotioCDAWindow;
class IntervalNotioCDAData;

#include "NotioData.h"

class NotioCDAData  {
public :
    NotioCDAData();
    int setData(RideFile *ride);

    int eTotIndex;
    int speedIndex;
    int rawCDAIndex;
    int ekfCDAIndex;
    int winCDAIndex;
    int ekfAltIndex;
    int bcvAltIndex;
    int huAltIndex;
    int airpressureIndex;
    int airDensityIndex;
    int windIndex;
    XDataSeries *cdaSeries = nullptr;
    int dataPoints;
    double m_TotalWeight = 80.0;
    double m_RiderFactor = 1.39;
    double m_Crr = 0.0040;
    double m_MechEff = 1.0;
    double m_RiderExponent = -0.05;
    double m_InertiaFactor = 1.15;

    static constexpr int WINDAVG = 7;

    int getNumPoints();
    double getSecs(int);
    double getWind(int, const int iOrder = WINDAVG);
    double getVirtualWind(int, double, double, const int iOrder = WINDAVG);
    double getEkfCDA(int);
    double getCorrectedAlt(int);
    double getOriginalAlt(int);
    double getHeadUnitAlt(int);
    double getWatts(int);
    double getDP(int);
    double getKPH(int);
    double getHeadWind(int);
    double getKM(int);

    // CdA calculation methods.
    double intervalCDA(int iStart, int iEnd);
    double intervalCDA(IntervalItem *iInterval);

    RideFile *thisRide = nullptr;
};

class NotioCDAPlot : public QwtPlot {

    Q_OBJECT
    G_OBJECT


    public:
        NotioCDAPlot( NotioCDAWindow *, Context * );
    bool byDistance() const { return bydist; }
    bool isShowingLegend() const { return m_showLegend; }
    bool useMetricUnits;  // whether metric units are used (or imperial)
    void setData(RideItem *_rideItem, bool new_zoom);
    void setAxisTitle(int axis, QString label);
    void rideSelected(RideItem *iRideItem);

    void refreshIntervalMarkers();

private:

    static constexpr int cColorAlpha = 100;
    NotioCDAData NotioCDAData;
    //NKC2
    //NKC1
    bool GarminON;
    bool WindOn;
    bool m_showLegend = true;
    Context *context;
    NotioCDAWindow *parent;

    LTMToolTip      *tooltip;
    LTMCanvasPicker *_canvasPicker; // allow point selection/hover
	
	// Axes definitions
    const QwtAxisId cCdaAxisLeft = {QwtPlot::yLeft, 0};
    const QwtAxisId cAltAxisRight = {QwtPlot::yRight, 0};
    const QwtAxisId cWindAxisRight = {QwtPlot::yRight, 1};
    const QwtAxisId cDebugAxisRight = {QwtPlot::yRight, 2};

    void adjustEoffset();

public slots:

    void setConstantAlt(int value);
    //NKC2
    void setGarminON(int value);
    //NKC1
    void setWindOn(int value);
    void setByDistance(int value);
    void setShowLegend(bool iState);
    void configChanged(qint32);

    void pointHover( QwtPlotCurve *, int );
    void intervalHover(IntervalItem *iInterval);

signals:

protected:
    friend class ::NotioCDAWindow;
    friend class ::IntervalNotioCDAData;


    QwtPlotGrid *grid;
    QVector<QwtPlotMarker*> d_mrk;

    // One curve to plot in the Course Profile:
    QwtPlotCurve *windCurve;   // virtual elevation curve
    QwtPlotCurve *windAverageCurve;   // virtual elevation curve
    QwtPlotCurve *cdaAverageCurve;   // virtual elevation curve
    QwtPlotCurve *vWindCurve;   // virtual elevation curve
    QwtPlotMarker *windRef = nullptr;       // Wind zero reference marker
    QwtPlotMarker *cdaOffsetRef = nullptr;  // CdA offset reference marker
    //NKC1
    //NKC2
    QwtPlotCurve *altCurve;    // Corrected elevation curve
    QwtPlotCurve *altInCurve;    // Original altitude curve
    QwtPlotCurve *altHeadUnitCurve;    // Garmin, Head Unit curve

    QwtPlotCurve *intervalHighlighterCurve;  // highlight selected intervals on the Plot
    QwtPlotCurve *m_intervalHoverCurve = nullptr;
    IntervalItem *m_hovered = nullptr;

    RideItem *rideItem;

    QVector<double> hrArray;
    QVector<double> wattsArray;
    QVector<double> speedArray;
    QVector<double> cadArray;

    // We store virtual elevation, time, altitude,and distance:
    QVector<double> windArray;
    QVector<double> windAverageArray;
    QVector<double> cdaAverageArray;
    QVector<double> vWindArray;
    QVector<double> syncArray;
    //NKC1
    //NKC2
    QVector<double> altArray;
    QVector<double> altInArray;
    QVector<double> altHeadUnitArray;
    QVector<double> timeArray;
    QVector<double> distanceArray;

    int smooth;
    bool bydist;
    bool constantAlt;
    int arrayLength;
    int iCrr;
    int iCda;

    double crr;
    double cda;
    double cdaRange;
    double totalMass; // Bike + Rider mass
    double riderFactor;
    double riderExponent;
    double eta;
    double windowSize;
    double cdaWindowSize;
    double timeOffset;
    double inertiaFactor;


    //double   slope(double, double, double, double, double, double, double);
    void     recalc(bool);
    void     setYMax(bool);
    void     setXTitle();
    void     setIntCrr(int);
    void     setIntCda(int);
    void     setIntCdaRange(int);
    void     setIntRiderFactor(int);
    void     setIntRiderExponent(int);
    void     setIntEta(int);
    void     setIntCalcWindow(int);
    void     setIntCDAWindow(int);
    void     setIntTimeOffset(int);
    void     setIntInertiaFactor(int);
    void     setIntTotalMass(int);
    void     setCdaOffsetMarker(double);
    double   getCrr() const { return (double)crr; }
    double   getCda() const { return (double)cda; }
    double   getCdaRange() const { return (double)cdaRange; }
    double   getTotalMass() const { return (double)totalMass; }
    double   getRiderFactor() const { return (double)riderFactor; }
    double   getRiderExponent() const { return (double)riderExponent; }
    double   getEta() const { return (double)eta; }
    double   getCalcWindow() const { return (double)windowSize; }
    double   getCDAWindow() const { return (double)cdaWindowSize; }
    double   getTimeOffset() const { return (double)timeOffset; }
    double   getInertiaFactor() const { return (double)inertiaFactor; }
    int      intCrr() const { return (int)( crr * 1000000  ); }
    int      intCda() const { return (int)( cda * 10000); }
    int      intCdaRange() const { return (int)( cdaRange * 10000); }
    int      intTotalMass() const { return (int)( totalMass * 100); }
    int      intRiderFactor() const { return (int)( riderFactor * 10000); }
    int      intRiderExponent() const { return (int)( riderExponent * 10000); }
    int      intEta() const { return (int)( eta * 10000); }
    int      intCalcWindow() const { return (int)( windowSize ); }
    int      intCDAWindow() const { return (int)( cdaWindowSize ); }
    int      intTimeOffset() const { return (int)( timeOffset ); }
    int      intInertiaFactor() const { return (int)( inertiaFactor*10000 ); }
    QString  estimateCdACrr(RideItem* rideItem);
    QString  estimateFactExp(RideItem* rideItem, const double &iExponent, double &oFactor);
};

#endif // _GC_NotioCDA_h

