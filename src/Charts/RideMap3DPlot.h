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

#ifndef RIDEMAP3DPLOT_H
#define RIDEMAP3DPLOT_H

#include <qwt_plot.h>
#include <QtGui>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class RideItem;
class RideFile;
class IntervalItem;
class LTMToolTip;
class LTMCanvasPicker;
class QwtPlotPicker;
class AllPlot;
class Context;
class QColor;
class UserData;

///////////////////////////////////////////////////////////////////////////////////
/// \brief The RideMap3DPlot class
///        This class creates a small plot under the 3D map.
///
///        This graph displays the altitude and the current displayed metric for
///        the entire ride.
///
///        As the mouse hover the interval tree view elements, the each interval
///        element is highlighted on the plot. Same thing goes when it is selected.
///
///        The map 3D small plot allows to create new intervals by selecting a
///        portion of the ride with a drag event.
///////////////////////////////////////////////////////////////////////////////////
class RideMap3DPlot : public QwtPlot
{
    Q_OBJECT

    public:
        RideMap3DPlot(Context * iContext = nullptr, QWidget *iParent = nullptr);

        int smoothing() const { return m_smooth; }
        void setData(RideItem *iRideItem);
        void setAxisTitle(QwtAxisId iAxisId, QString iLabel);
        void recalc();
        void setYMax();
        void setXTitle();

        void setMetric(QString iSeriesSymbol);
        void setMetric(UserData *iCustom);

    signals:
        void simpleButtonClick();
        void endNewSelection();
        void dragNewSelection();

    public slots:
        void setSmoothing(int iValue);
        void intervalHover(IntervalItem *iInterval);
        void refreshSelection();
        void intervalsChanged();
        void setAltitudeVisible(const bool iStatus = true);
        void setMetricVisible(const bool iStatus = true);

        // For Tooltip
        void pointHover(QwtPlotCurve*, int);
        void pointClicked(QwtPlotCurve*, int);

        void plotPickerSelected(const QPoint &iPos);
        void plotPickerMoved(const QPoint &iPos);
        void plotPickerStarted();
        void plotPickerStopped();

        void hideSelection();
        void clearSelection();

        void configChanged(qint32);

    protected:
        enum eMetricType { STANDARD, USERDATA };

        void setCurrentMetricArray();
        void setSelection(double iValueX, bool iNewInterval);
        void selectNewIntervalOnly();

        unsigned int getNextSelectionId(RideItem *iCurrentRide);
        void checkIntervalsIntegrity();

        bool event(QEvent *iEvent);

        Context *m_context = nullptr;
        QWidget *m_parent = nullptr;
        RideItem *m_currentRideItem = nullptr;

        // Curves
        QwtPlotCurve *m_altCurve = nullptr;
        QwtPlotCurve *m_metricCurve = nullptr;
        QVector<double> m_altArray;
        QVector<double> m_timeArray;
        QVector<double> m_metricArray;
        QVector<QwtPlotCurve*> m_timeCurves;
        int m_arrayLength = 0;
        QVector<int> m_interArray;
        eMetricType m_metricType;
        QString m_metricSeriesSymbol;
        QString m_metricUnit;
        UserData *m_metricUserData = nullptr;
        int m_smooth = 1;

        // Highlighting intervals.
        QwtPlotCurve *m_intervalSelection = nullptr;
        QwtPlotCurve *m_intervalHover = nullptr;
        IntervalItem *m_hovered = nullptr;
        double m_intervalMaxY = 0;
        double m_intervalMinY = 0;

        // Tool tip.
        LTMToolTip *m_tooltip = nullptr;
        LTMCanvasPicker *m_canvasPicker = nullptr;

        // Markers.
        QwtPlotMarker *m_curveMarker = nullptr;
        QwtPlotMarker *m_selMarker1 = nullptr;
        QwtPlotMarker *m_selMarker2 = nullptr;

        // New interval variables.
        bool m_selDrag = false;
        bool m_createNewInterval = false;
        static constexpr double kMinInterval = 0.4999999;
        IntervalItem *m_newInterval = nullptr;
        static QMap<RideItem*, QMap<unsigned int, bool> > m_mapRideIntervalNewName;
};

#endif // MAP3DSMALLPLOT_H
