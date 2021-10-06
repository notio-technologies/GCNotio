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

#ifndef _GC_NotioCDAWindow_h
#define _GC_NotioCDAWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QMessageBox>

class NotioCDAPlot;
class Context;
class QCheckBox;
class QwtPlotZoomer;
class QwtPlotPicker;
class QLineEdit;
class RideItem;
class IntervalItem;
class QGroupBox;

#define PCRRCOLOR 19 //CPOWER
#define PTOTCOLOR 18 //CPOWER
#define PACCCOLOR 17 //CSPEED
#define PAIRCOLOR 16 //CSPEED
#define PALTCOLOR 15 //CSPEED
#define PDIFFCOLOR 14 //CSPEED

class NotioCDAWindow : public GcChartWindow {
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(bool windChecked READ isWindOn WRITE setWindOn USER true)
    Q_PROPERTY(int byDistance READ isByDistance WRITE setByDistance USER true)

#ifndef Q_OS_MAC
    static constexpr int cGroupBoxTitleSpace = 2;
#else
    static constexpr int cGroupBoxTitleSpace = 1;
#endif

    public:
        NotioCDAWindow(Context *context);
        ~NotioCDAWindow();
    void setData(RideItem *ride);
    double getCanvasTop() const;
    double getCanvasBottom() const;
    bool isWindOn();
    int isByDistance();
    bool hasParamToSave() { return btnSave->isEnabled(); }

    QSlider *calcWindowSlider;
    QSlider *cdaWindowSlider;
    QSlider *timeOffsetSlider;
    QSlider *inertiaFactorSlider;

public slots:

    void setCrrFromSlider();
    void setCrrFromValue(double);
    void setCdaFromSlider();
    void setCdaRangeFromSlider();
    void setCdaFromValue(double);
    void setCdaRangeFromValue(double);
    void setTotalMassFromSlider();
    void setTotalMassFromValue(double);
    void setFactorFromSlider();
    void setFactorFromValue(double);
    void setExponentFromSlider();
    void setExponentFromValue(double);
    void setEtaFromSlider();
    void setEtaFromValue(double);
    void setComment(const QString text);
    void setCalcWindowFromSlider();
    void setCalcWindowFromValue(double);
    void setCDAWindowFromSlider();
    void setCDAWindowFromValue(double);
    void setTimeOffsetFromSlider();
    void setTimeOffsetFromValue(double);
    void setInertiaFactorFromSlider();
    void setInertiaFactorFromValue(double);
    void doEstCalibFact();
    void doEstCdACrr();
    void setGarminON(int value);
    void setWindOn(int value);
    void setByDistance(int value);
    void rideSelected();
    void zoomChanged();
    void zoomInterval(IntervalItem *); // zoom into a specified interval
    void zoomOut();
    void configChanged(qint32);
    void intervalSelected();
    void saveParametersInRide();

protected:

    Context *context;
    NotioCDAPlot *aerolab;
    QwtPlotZoomer *allZoomer;
    bool devOptions;
    bool m_isBlank = false;
    bool m_mainParamChanged = false;

    // labels
    QLabel *crrLabel;
    QLabel *cdaLabel;
    QLabel *cdaRangeLabel;
    QLabel *etaLabel;
    QLabel *commentLabel;
    QLabel *mLabel;
    QLabel *factorLabel;
    QLabel *exponentLabel;
    QLabel *calcWindowLabel;
    QLabel *cdaWindowLabel;
    QLabel *timeOffsetLabel;
    QLabel *inertiaFactorLabel;

    // Settings and parameters layout Groups
    QGroupBox *m_scaleSettings = nullptr;
    QGroupBox *m_parameters = nullptr;
    QGroupBox *m_devSectionGroup = nullptr;

    //NKC2
    QPushButton *btnEstCdACrr;

    QCheckBox *GarminON;
    QCheckBox *WindOn;
    QComboBox *comboDistance = nullptr;

    // Bike parameter controls:
    QSlider *crrSlider;
    QDoubleSpinBox *crrDoubleSpinBox = nullptr;
    QSlider *cdaSlider;
    QSlider *cdaRangeSlider;
    QDoubleSpinBox *cdaDoubleSpinBox = nullptr;
    QDoubleSpinBox *cdaRangeDoubleSpinBox = nullptr;
    QSlider *mSlider;
    QDoubleSpinBox *mMassDoubleSpinBox = nullptr;  // Total Mass
    QSlider *factorSlider;
    QDoubleSpinBox *factorDoubleSpinBox = nullptr;
    QSlider *exponentSlider;
    QDoubleSpinBox *exponentDoubleSpinBox = nullptr;
    QSlider *etaSlider;
    QDoubleSpinBox *etaDoubleSpinBox = nullptr;

    QDoubleSpinBox *calcWindowDoubleSpinBox = nullptr;
    QDoubleSpinBox *cdaWindowDoubleSpinBox = nullptr;
    QDoubleSpinBox *timeOffsetDoubleSpinBox = nullptr;
    QDoubleSpinBox *inertiaFactorDoubleSpinBox = nullptr;

    QLineEdit *commentEdit;

    QPushButton *btnSave;

    void refresh(RideItem *iRideItem, bool iNewZoom = false);
    void loadRideParameters();
    void setParamChanged(bool iEnable = true);
    QMessageBox *m_msgBox = nullptr;
};

#endif // _GC_NotioCDAWindow_h
