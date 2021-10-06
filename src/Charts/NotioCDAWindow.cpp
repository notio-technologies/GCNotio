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


#include "Context.h"
#include "Tab.h"
#include "Athlete.h"
#include "Units.h"
#include "NotioCDAWindow.h"
#include "NotioCDA.h"
#include "NotioComputeFunctions.h"
#include "GcUpgrade.h"
#include "TabView.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <QtGui>
#include <qwt_plot_zoomer.h>
#include <QListWidget>
#include <QGroupBox>

#include <iostream>
using namespace std;
using namespace NotioComputeFunctions;

NotioCDAWindow::NotioCDAWindow(Context *context) :
    GcChartWindow(context), context(context) {
    setControls(nullptr);

    // NotioCDA tab layout:
    QVBoxLayout *vLayout      = new QVBoxLayout;
    QVBoxLayout *cLayout      = new QVBoxLayout; //XX1

    devOptions = context->isDevMode(); // true; // TRUE means ON

    // Plot:
    aerolab = new NotioCDAPlot(this, context);

    HelpWhatsThis *help = new HelpWhatsThis(aerolab);
    aerolab->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Aerolab));

    QFontMetrics metrics(QApplication::font());
    int labelWidth0 = metrics.width(tr("Offset")) + 10;
    int labelWidth1 = metrics.width(tr("Total Mass (kg)")) + 10;
    int labelWidth2 = metrics.width(QString("%1 (%2p)").arg(tr("Mechanical Efficiency")).arg(QChar(0xB7, 0x03))) + 10;

    QString wToolTipText;

    // Row1 controls layout:

    QHBoxLayout *row1Controls  =  new QHBoxLayout; //XX2

    m_scaleSettings = new QGroupBox(tr("CdA Scale Settings"));

    QVBoxLayout *c1Layout = new QVBoxLayout;

    // CdA:
    QHBoxLayout *cdaLayout = new QHBoxLayout;
    wToolTipText = tr("CdA Scale Offset");
    cdaLabel = new QLabel(tr("Offset"), this);
    cdaLabel->setToolTip(wToolTipText);
    cdaLabel->setFixedWidth(labelWidth0);
    cdaDoubleSpinBox = new QDoubleSpinBox(this);
    cdaDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    cdaDoubleSpinBox->setDecimals(3);
    cdaDoubleSpinBox->setSingleStep(0.01);
    cdaDoubleSpinBox->setRange(-1.0, 1.0);
    cdaDoubleSpinBox->setFixedWidth(50);
    cdaDoubleSpinBox->setValue(aerolab->getCda());
    cdaDoubleSpinBox->setToolTip(wToolTipText);
    cdaSlider = new QSlider(Qt::Horizontal);
    cdaSlider->setTickPosition(QSlider::TicksBelow);
    cdaSlider->setTickInterval(2000);
    cdaSlider->setMinimum(-10000);
    cdaSlider->setMaximum(10000);
    cdaSlider->setValue(aerolab->intCda());
    cdaSlider->setToolTip(wToolTipText);
    cdaLayout->addWidget( cdaLabel );
    cdaLayout->addWidget( cdaDoubleSpinBox );
    cdaLayout->addWidget( cdaSlider );

    c1Layout->addLayout( cdaLayout );

    // CdA:
    QHBoxLayout *cdaRangeLayout = new QHBoxLayout;
    wToolTipText = tr("CdA Scale Range");
    cdaRangeLabel = new QLabel(tr("Range"), this);
    cdaRangeLabel->setToolTip(wToolTipText);
    cdaRangeLabel->setFixedWidth(labelWidth0);
    cdaRangeDoubleSpinBox = new QDoubleSpinBox(this);
    cdaRangeDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    cdaRangeDoubleSpinBox->setDecimals(3);
    cdaRangeDoubleSpinBox->setSingleStep(0.01);
    cdaRangeDoubleSpinBox->setRange(0.001, 0.6);
    cdaRangeDoubleSpinBox->setFixedWidth(50);
    cdaRangeDoubleSpinBox->setValue(aerolab->getCda());
    cdaRangeDoubleSpinBox->setToolTip(wToolTipText);

    cdaRangeSlider = new QSlider(Qt::Horizontal);
    cdaRangeSlider->setTickPosition(QSlider::TicksBelow);
    cdaRangeSlider->setTickInterval(800);
    cdaRangeSlider->setMinimum(10);
    cdaRangeSlider->setMaximum(6000);
    cdaRangeSlider->setValue(aerolab->intCda());
    cdaRangeSlider->setToolTip(wToolTipText);
    cdaRangeLayout->addWidget( cdaRangeLabel );
    cdaRangeLayout->addWidget( cdaRangeDoubleSpinBox );
    cdaRangeLayout->addWidget( cdaRangeSlider );

    c1Layout->addLayout( cdaRangeLayout );

    m_scaleSettings->setLayout(c1Layout);

    m_parameters = new QGroupBox(tr("Parameters"));

    QHBoxLayout *wParamLayout = new QHBoxLayout;

    QVBoxLayout *c2Layout = new QVBoxLayout;

    // Total mass:
    QHBoxLayout *mLayout = new QHBoxLayout;
    wToolTipText = tr("Rider Total Mass");
    mLabel = new QLabel(tr("Total Mass (kg)"), this);
    mLabel->setToolTip(wToolTipText);
    mLabel->setFixedWidth(labelWidth1);
    mMassDoubleSpinBox = new QDoubleSpinBox(this);
    mMassDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    mMassDoubleSpinBox->setDecimals(2);
    mMassDoubleSpinBox->setRange(35, 150);
    mMassDoubleSpinBox->setFixedWidth(50);
    mMassDoubleSpinBox->setValue(aerolab->getTotalMass());
    mMassDoubleSpinBox->setToolTip(wToolTipText);
    mSlider = new QSlider(Qt::Horizontal);
    mSlider->setTickPosition(QSlider::TicksBelow);
    mSlider->setTickInterval(1000);
    mSlider->setMinimum(3500);
    mSlider->setMaximum(15000);
    mSlider->setValue(aerolab->intTotalMass());
    mSlider->setToolTip(wToolTipText);
    mLayout->addWidget( mLabel );
    mLayout->addWidget( mMassDoubleSpinBox );
    mLayout->addWidget( mSlider );

    c2Layout->addLayout( mLayout );

    // Crr:
    QHBoxLayout *crrLayout = new QHBoxLayout;
    wToolTipText = tr("Rolling Resistance");
    crrLabel = new QLabel(tr("Crr"), this);
    crrLabel->setToolTip(wToolTipText);
    crrLabel->setFixedWidth(labelWidth1);
    crrDoubleSpinBox = new QDoubleSpinBox(this);
    crrDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    crrDoubleSpinBox->setDecimals(4);
    crrDoubleSpinBox->setSingleStep(0.001);
    crrDoubleSpinBox->setRange(0.001, 0.015);
    crrDoubleSpinBox->setFixedWidth(50);
    crrDoubleSpinBox->setValue(aerolab->getCrr());
    crrDoubleSpinBox->setToolTip(wToolTipText);
    crrSlider = new QSlider(Qt::Horizontal);
    crrSlider->setTickPosition(QSlider::TicksBelow);
    crrSlider->setTickInterval(1000);
    crrSlider->setMinimum(1000);
    crrSlider->setMaximum(15000);
    crrSlider->setValue(aerolab->intCrr());
    crrSlider->setToolTip(wToolTipText);
    crrLayout->addWidget( crrLabel );
    crrLayout->addWidget( crrDoubleSpinBox );
    crrLayout->addWidget( crrSlider );

    c2Layout->addLayout( crrLayout );

    wParamLayout->addLayout(c2Layout);

    QVBoxLayout *c3Layout = new QVBoxLayout;

    // Eta:
    QHBoxLayout *etaLayout = new QHBoxLayout;
    wToolTipText = tr("Mechanical Efficiency");
    etaLabel = new QLabel(QString("%1 (%2<sub>p</sub>)").arg(wToolTipText).arg(QChar(0xB7, 0x03)), this);    // ETA is a greek character.
    etaLabel->setToolTip(wToolTipText);
    etaLabel->setFixedWidth(labelWidth2);
    etaDoubleSpinBox = new QDoubleSpinBox(this);
    etaDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    etaDoubleSpinBox->setDecimals(4);
    etaDoubleSpinBox->setSingleStep(0.01);
    etaDoubleSpinBox->setRange(0.8, 1.2);
    etaDoubleSpinBox->setFixedWidth(50);
    etaDoubleSpinBox->setValue(aerolab->getEta());
    etaDoubleSpinBox->setToolTip(wToolTipText);
    etaSlider = new QSlider(Qt::Horizontal);
    etaSlider->setTickPosition(QSlider::TicksBelow);
    etaSlider->setTickInterval(500);
    etaSlider->setMinimum(8000);
    etaSlider->setMaximum(12000);
    etaSlider->setValue(aerolab->intEta());
    etaSlider->setToolTip(wToolTipText);
    etaLayout->addWidget( etaLabel );
    etaLayout->addWidget( etaDoubleSpinBox );
    etaLayout->addWidget( etaSlider );

    c3Layout->addLayout( etaLayout );

    QHBoxLayout *factorLayout = new QHBoxLayout;
    wToolTipText = tr("Formerly Coefficient 1");
    factorLabel = new QLabel(tr("Calibration Factor"), this);
    factorLabel->setToolTip(wToolTipText);
    factorLabel->setFixedWidth(labelWidth2);
    factorDoubleSpinBox = new QDoubleSpinBox(this);
    factorDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    factorDoubleSpinBox->setDecimals(3);
    factorDoubleSpinBox->setSingleStep(0.01);
    factorDoubleSpinBox->setRange(1.0, 2.0);
    factorDoubleSpinBox->setFixedWidth(50);
    factorDoubleSpinBox->setValue(aerolab->getRiderFactor());
    factorDoubleSpinBox->setToolTip(wToolTipText);
    factorSlider = new QSlider(Qt::Horizontal);
    factorSlider->setTickPosition(QSlider::TicksBelow);
    factorSlider->setTickInterval(1000);
    factorSlider->setMinimum(10000);
    factorSlider->setMaximum(20000);
    factorSlider->setValue(aerolab->intRiderFactor());
    factorSlider->setToolTip(wToolTipText);
    factorLayout->addWidget( factorLabel );
    factorLayout->addWidget( factorDoubleSpinBox );
    factorLayout->addWidget( factorSlider );

    c3Layout->addLayout(factorLayout);

    wParamLayout->addLayout(c3Layout);
    m_parameters->setLayout(wParamLayout);

    QVBoxLayout *c4Layout = new QVBoxLayout;
    wToolTipText = tr("Show/Hide Wind curves");
    WindOn  = new QCheckBox(tr("Wind"), this);
    WindOn->setToolTip(wToolTipText);
    WindOn->setCheckState(Qt::Unchecked);

    c4Layout->addWidget(WindOn);

    wToolTipText = tr("Estimate calibration factor (formerly Coefficient 1)");
    QPushButton *btnEstCalibFact = new QPushButton(tr("Estimate Factor"), this);
    btnEstCalibFact->setToolTip(wToolTipText);
    c4Layout->addWidget(btnEstCalibFact);

    wToolTipText = tr("Save modified parameters");
    btnSave = new QPushButton(tr("&Save parameters"), this);
    btnSave->setToolTip(wToolTipText);
    btnSave->setEnabled(false);
    c4Layout->addWidget(btnSave);

    row1Controls->addWidget(m_scaleSettings, 1);
    row1Controls->addWidget(m_parameters, 3);
    row1Controls->addLayout(c4Layout, 0);

    QHBoxLayout *rowDevControls  =  new QHBoxLayout;
    m_devSectionGroup = new QGroupBox(tr("Development Parameters"));
    m_devSectionGroup->setVisible(devOptions);

    QHBoxLayout *wParametersLayout = new QHBoxLayout;
    QVBoxLayout *wC1DevLayout = new QVBoxLayout;

    QHBoxLayout *exponentLayout = new QHBoxLayout;
    exponentLabel = new QLabel(tr("Exponent"), this);
    exponentLabel->setFixedWidth(labelWidth1);
    exponentDoubleSpinBox = new QDoubleSpinBox(this);
    exponentDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    exponentDoubleSpinBox->setDecimals(4);
    exponentDoubleSpinBox->setSingleStep(0.001);
    exponentDoubleSpinBox->setRange(-1.0, 1.0);
    exponentDoubleSpinBox->setFixedWidth(55);
    exponentDoubleSpinBox->setValue(aerolab->getRiderExponent());
    exponentSlider = new QSlider(Qt::Horizontal);
    exponentSlider->setTickPosition(QSlider::TicksBelow);
    exponentSlider->setTickInterval(1000);
    exponentSlider->setMinimum(-10000);
    exponentSlider->setMaximum(10000);
    exponentSlider->setValue(aerolab->intRiderExponent());
    exponentLayout->addWidget(exponentLabel);
    exponentLayout->addWidget(exponentDoubleSpinBox);
    exponentLayout->addWidget(exponentSlider);

    wC1DevLayout->addLayout(exponentLayout);

    // Window offset
    QHBoxLayout *timeOffsetLayout = new QHBoxLayout;
    timeOffsetLabel = new QLabel(tr("Time Offset(s)"), this);
    timeOffsetLabel->setFixedWidth(labelWidth1);
    timeOffsetDoubleSpinBox = new QDoubleSpinBox(this);
    timeOffsetDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    timeOffsetDoubleSpinBox->setDecimals(0);
    timeOffsetDoubleSpinBox->setSingleStep(1);
    timeOffsetDoubleSpinBox->setRange(-10.0, 10.0);
    timeOffsetDoubleSpinBox->setFixedWidth(55);
    timeOffsetDoubleSpinBox->setValue(aerolab->getTimeOffset());
    timeOffsetSlider = new QSlider(Qt::Horizontal);
    timeOffsetSlider->setTickPosition(QSlider::TicksBelow);
    timeOffsetSlider->setTickInterval(10);
    timeOffsetSlider->setMinimum(-10);
    timeOffsetSlider->setMaximum(10);
    timeOffsetSlider->setValue(aerolab->intTotalMass());
    timeOffsetLayout->addWidget(timeOffsetLabel);
    timeOffsetLayout->addWidget(timeOffsetDoubleSpinBox);
    timeOffsetLayout->addWidget(timeOffsetSlider);

    wC1DevLayout->addLayout(timeOffsetLayout);

    QVBoxLayout *wC2DevLayout = new QVBoxLayout;

    QHBoxLayout *calcWindowLayout = new QHBoxLayout;
    calcWindowLabel = new QLabel(tr("Calc Win (s)"), this);
    calcWindowLabel->setVisible(false);     // Hidden for the moment.
    calcWindowDoubleSpinBox = new QDoubleSpinBox(this);
    calcWindowDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    calcWindowDoubleSpinBox->setDecimals(0);
    calcWindowDoubleSpinBox->setSingleStep(5);
    calcWindowDoubleSpinBox->setRange(1.0, 90.0);
    calcWindowDoubleSpinBox->setFixedWidth(50);
    calcWindowDoubleSpinBox->setValue(aerolab->getCalcWindow());
    calcWindowDoubleSpinBox->setVisible(false);     // Hidden for the moment.
    calcWindowSlider = new QSlider(Qt::Horizontal);
    calcWindowSlider->setTickPosition(QSlider::TicksBelow);
    calcWindowSlider->setTickInterval(10);
    calcWindowSlider->setMinimum(1);
    calcWindowSlider->setMaximum(90);
    calcWindowSlider->setValue(aerolab->intCalcWindow());
    calcWindowSlider->setVisible(false);     // Hidden for the moment.
    calcWindowLayout->addWidget(calcWindowLabel);
    calcWindowLayout->addWidget(calcWindowDoubleSpinBox);
    calcWindowLayout->addWidget(calcWindowSlider);

    wC2DevLayout->addLayout(calcWindowLayout);

    QHBoxLayout *cdaWindowLayout = new QHBoxLayout;
    cdaWindowLabel = new QLabel(tr("CDA Win (s)"), this);
    cdaWindowLabel->setVisible(false);     // Hidden for the moment.
    cdaWindowDoubleSpinBox = new QDoubleSpinBox(this);
    cdaWindowDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    cdaWindowDoubleSpinBox->setDecimals(0);
    cdaWindowDoubleSpinBox->setSingleStep(5);
    cdaWindowDoubleSpinBox->setRange(1.0, 90.0);
    cdaWindowDoubleSpinBox->setFixedWidth(50);
    cdaWindowDoubleSpinBox->setValue(aerolab->getCDAWindow());
    cdaWindowDoubleSpinBox->setVisible(false);     // Hidden for the moment.
    cdaWindowSlider = new QSlider(Qt::Horizontal);
    cdaWindowSlider->setTickPosition(QSlider::TicksBelow);
    cdaWindowSlider->setTickInterval(10);
    cdaWindowSlider->setMinimum(1);
    cdaWindowSlider->setMaximum(90);
    cdaWindowSlider->setValue(aerolab->intCalcWindow());
    cdaWindowSlider->setVisible(false);     // Hidden for the moment.
    cdaWindowLayout->addWidget(cdaWindowLabel);
    cdaWindowLayout->addWidget(cdaWindowDoubleSpinBox);
    cdaWindowLayout->addWidget(cdaWindowSlider);

    wC2DevLayout->addLayout(cdaWindowLayout);

    QVBoxLayout *wC3DevLayout = new QVBoxLayout;

    QHBoxLayout *inertiaFactorLayout = new QHBoxLayout;
    inertiaFactorLabel = new QLabel(tr("Inertia"), this);
    inertiaFactorLabel->setVisible(false);     // Hidden for the moment.
    inertiaFactorDoubleSpinBox = new QDoubleSpinBox(this);
    inertiaFactorDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    inertiaFactorDoubleSpinBox->setDecimals(4);
    inertiaFactorDoubleSpinBox->setSingleStep(0.001);
    inertiaFactorDoubleSpinBox->setRange(0.8, 1.2);
    inertiaFactorDoubleSpinBox->setFixedWidth(50);
    inertiaFactorDoubleSpinBox->setValue(aerolab->getInertiaFactor());
    inertiaFactorDoubleSpinBox->setVisible(false);     // Hidden for the moment.
    inertiaFactorSlider = new QSlider(Qt::Horizontal);
    inertiaFactorSlider->setTickPosition(QSlider::TicksBelow);
    inertiaFactorSlider->setTickInterval(1000);
    inertiaFactorSlider->setMinimum(8000);
    inertiaFactorSlider->setMaximum(12000);
    inertiaFactorSlider->setValue(aerolab->intTotalMass());
    inertiaFactorSlider->setVisible(false);     // Hidden for the moment.
    inertiaFactorLayout->addWidget(inertiaFactorLabel);
    inertiaFactorLayout->addWidget(inertiaFactorDoubleSpinBox);
    inertiaFactorLayout->addWidget(inertiaFactorSlider);

    wC3DevLayout->addLayout(inertiaFactorLayout);

    QVBoxLayout *wC4DevLayout = new QVBoxLayout;

    GarminON  = new QCheckBox(tr("Garmin altitude"), this);
    GarminON->setCheckState(Qt::Unchecked);
    //GarminON->setVisible(devOptions);
    GarminON->setVisible(false);     // Hidden for the moment.
    wC4DevLayout->addWidget(GarminON);

    btnEstCdACrr = new QPushButton(tr("&Estimate CdA and Crr"), this);
    //btnEstCdACrr->setVisible(devOptions);
    btnEstCdACrr->setVisible(false);     // Hidden for the moment.
    wC4DevLayout->addWidget(btnEstCdACrr);

    wParametersLayout->addLayout(wC1DevLayout);
    wParametersLayout->addLayout(wC2DevLayout);
    wParametersLayout->addLayout(wC3DevLayout);

    m_devSectionGroup->setLayout(wParametersLayout);

    rowDevControls->addWidget(m_devSectionGroup, 3);
    rowDevControls->addLayout(wC4DevLayout, 0);

    QHBoxLayout *row2Controls  =  new QHBoxLayout; //XX2

    comboDistance = new QComboBox();
    comboDistance->addItem(tr("X Axis Shows Time"));
    comboDistance->addItem(tr("X Axis Shows Distance"));
    comboDistance->setCurrentIndex(1);
    row2Controls->addWidget(comboDistance);

    QHBoxLayout *commentLayout = new QHBoxLayout;
    wToolTipText = tr("Activity comment");
    commentLabel = new QLabel(tr("Comment"), this);
    commentLabel->setToolTip(wToolTipText);
    commentEdit = new QLineEdit();
    commentEdit->setToolTip(wToolTipText);
    commentLayout->addWidget(commentLabel);
    commentLayout->addWidget(commentEdit);
    row2Controls->addLayout(commentLayout);

    // Assemble controls layout:
    cLayout->addLayout(row1Controls);
    cLayout->addLayout(rowDevControls);
    cLayout->addLayout(row2Controls);

    // Zoomer:
    allZoomer = new QwtPlotZoomer(aerolab->canvas());
    allZoomer->setRubberBand(QwtPicker::RectRubberBand);
    allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    allZoomer->setEnabled(true);
    allZoomer->setMousePattern( QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier );
    allZoomer->setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton );

    // Create message box.
    m_msgBox = new QMessageBox(this);

    m_msgBox->setWindowTitle(tr("Notio CdA Analysis"));
    m_msgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    m_msgBox->setButtonText(QMessageBox::Ok, tr("Compute"));
    m_msgBox->setIcon(QMessageBox::Information);
    m_msgBox->setWindowModality(Qt::WindowModality::WindowModal);

    // SIGNALs to SLOTs:
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideSaved(RideItem*)), this, SLOT(rideSelected()));
    connect(crrSlider, SIGNAL(valueChanged(int)),this, SLOT(setCrrFromSlider()));
    connect(crrDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCrrFromValue(double)));
    connect(cdaSlider, SIGNAL(valueChanged(int)), this, SLOT(setCdaFromSlider()));
    connect(cdaRangeSlider, SIGNAL(valueChanged(int)), this, SLOT(setCdaRangeFromSlider()));
    connect(cdaDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCdaFromValue(double)));
    connect(cdaRangeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCdaRangeFromValue(double)));
    connect(mSlider, SIGNAL(valueChanged(int)),this, SLOT(setTotalMassFromSlider()));
    connect(mMassDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setTotalMassFromValue(double)));
    connect(factorSlider, SIGNAL(valueChanged(int)), this, SLOT(setFactorFromSlider()));
    connect(factorDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setFactorFromValue(double)));
    connect(exponentSlider, SIGNAL(valueChanged(int)), this, SLOT(setExponentFromSlider()));
    connect(exponentDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setExponentFromValue(double)));
    connect(etaSlider, SIGNAL(valueChanged(int)), this, SLOT(setEtaFromSlider()));
    connect(etaDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setEtaFromValue(double)));
    connect(commentEdit, SIGNAL(textChanged(const QString)), this, SLOT(setComment(const QString)));
    connect(calcWindowSlider, SIGNAL(valueChanged(int)), this, SLOT(setCalcWindowFromSlider()));
    connect(calcWindowDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCalcWindowFromValue(double)));
    connect(cdaWindowSlider, SIGNAL(valueChanged(int)), this, SLOT(setCDAWindowFromSlider()));
    connect(cdaWindowDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCDAWindowFromValue(double)));
    connect(timeOffsetSlider, SIGNAL(valueChanged(int)), this, SLOT(setTimeOffsetFromSlider()));
    connect(timeOffsetDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setTimeOffsetFromValue(double)));
    connect(inertiaFactorSlider, SIGNAL(valueChanged(int)), this, SLOT(setInertiaFactorFromSlider()));
    connect(inertiaFactorDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setInertiaFactorFromValue(double)));
    //NKC1
    connect(GarminON, SIGNAL(stateChanged(int)), this, SLOT(setGarminON(int)));
    //NKC2
    connect(WindOn, SIGNAL(stateChanged(int)), this, SLOT(setWindOn(int)));
    connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(setByDistance(int)));
    connect(btnEstCalibFact, SIGNAL(clicked()), this, SLOT(doEstCalibFact()));
    connect(btnEstCdACrr, SIGNAL(clicked()), this, SLOT(doEstCdACrr()));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(saveParametersInRide()));
    connect(context, SIGNAL(configChanged(qint32)), aerolab, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(intervalSelected() ), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalZoom(IntervalItem*) ), this, SLOT(zoomInterval(IntervalItem*)));
    connect(context, SIGNAL(zoomOut()), SLOT(zoomOut()));
    connect(allZoomer, SIGNAL( zoomed(const QRectF) ), this, SLOT(zoomChanged()));


    // Build the tab layout:
    vLayout->addWidget(aerolab);
    vLayout->addLayout(cLayout);
    setChartLayout(vLayout);

    // tooltip on hover over point
    //************************************
    aerolab->tooltip = new LTMToolTip( QwtPlot::xBottom,
                                       QwtPlot::yLeft,
                                       QwtPicker::VLineRubberBand,
                                       QwtPicker::AlwaysOn,
                                       aerolab->canvas(),
                                       ""
                                       );
    aerolab->tooltip->setRubberBand( QwtPicker::VLineRubberBand );
    aerolab->tooltip->setMousePattern( QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ShiftModifier );
    aerolab->tooltip->setTrackerPen( QColor( Qt::black ) );
    QColor inv( Qt::white );
    inv.setAlpha( 0 );
    aerolab->tooltip->setRubberBandPen( inv );
    aerolab->tooltip->setEnabled( true );
    aerolab->_canvasPicker = new LTMCanvasPicker( aerolab );

    connect( aerolab->_canvasPicker, SIGNAL( pointHover( QwtPlotCurve*, int ) ),
             aerolab,                SLOT  ( pointHover( QwtPlotCurve*, int ) ) );


    configChanged(CONFIG_APPEARANCE); // pickup colors etc
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::~NotioCDAWindow
///        Destructor
///////////////////////////////////////////////////////////////////////////////
NotioCDAWindow::~NotioCDAWindow()
{
    // Delete the static message box.
    if (m_msgBox)
    {
        delete m_msgBox;
        m_msgBox = nullptr;
    }
}

void
NotioCDAWindow::zoomChanged()
{
    RideItem *ride = myRideItem;
    // Check if blank.
    if ((ride == nullptr) || m_isBlank)
    {
        qDebug() << "Do not refresh if page is blank.";
        return;
    }

    refresh(ride);
}


void
NotioCDAWindow::configChanged(qint32)
{
    allZoomer->setRubberBandPen(GColor(CPLOTSELECT));
    setProperty("color", GColor(CRIDEPLOTBACKGROUND));

    QPalette palette;

    palette.setColor(QPalette::Window, GColor(CRIDEPLOTBACKGROUND));
    palette.setColor(QPalette::Background, GColor(CRIDEPLOTBACKGROUND));

    // only change base if moved away from white plots
    // which is a Mac thing
#ifndef Q_OS_MAC
    if (GColor(CRIDEPLOTBACKGROUND) != Qt::white)
#endif
    {
        palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CRIDEPLOTBACKGROUND)));
        palette.setColor(QPalette::Window,  GColor(CRIDEPLOTBACKGROUND));
    }

    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)));
    setPalette(palette);
    aerolab->setPalette(palette);
    QString wGroupBoxStyle = QString::fromUtf8("QGroupBox { border:1px solid %1; border-radius:3px; margin-top: %2ex; }"
                                               "QGroupBox::title { color: %1; subcontrol-origin: margin; subcontrol-position:top left; left: 20px; padding: 0 5px; }")
                                               .arg(GCColor::invertColor(GColor(CRIDEPLOTBACKGROUND)).name())
                                               .arg(cGroupBoxTitleSpace);

    m_parameters->setStyleSheet(wGroupBoxStyle);
    m_scaleSettings->setStyleSheet(wGroupBoxStyle);
    m_devSectionGroup->setStyleSheet(wGroupBoxStyle);

    crrLabel->setPalette(palette);
    crrSlider->setPalette(palette);
    crrDoubleSpinBox->setPalette(palette);
    cdaLabel->setPalette(palette);
    cdaSlider->setPalette(palette);
    cdaDoubleSpinBox->setPalette(palette);
    cdaRangeLabel->setPalette(palette);
    cdaRangeSlider->setPalette(palette);
    cdaRangeDoubleSpinBox->setPalette(palette);
    etaLabel->setPalette(palette);
    etaSlider->setPalette(palette);
    etaDoubleSpinBox->setPalette(palette);
    commentLabel->setPalette(palette);
    commentEdit->setPalette(palette);
    mLabel->setPalette(palette);
    mSlider->setPalette(palette);
    mMassDoubleSpinBox->setPalette(palette);
    factorLabel->setPalette(palette);
    factorSlider->setPalette(palette);
    factorDoubleSpinBox->setPalette(palette);
    exponentLabel->setPalette(palette);
    exponentDoubleSpinBox->setPalette(palette);
    exponentSlider->setPalette(palette);
    calcWindowLabel->setPalette(palette);
    calcWindowDoubleSpinBox->setPalette(palette);
    calcWindowSlider->setPalette(palette);
    cdaWindowLabel->setPalette(palette);
    cdaWindowSlider->setPalette(palette);
    cdaWindowDoubleSpinBox->setPalette(palette);
    timeOffsetLabel->setPalette(palette);
    timeOffsetDoubleSpinBox->setPalette(palette);
    timeOffsetSlider->setPalette(palette);
    inertiaFactorLabel->setPalette(palette);
    inertiaFactorDoubleSpinBox->setPalette(palette);
    inertiaFactorSlider->setPalette(palette);

    GarminON->setPalette(palette);
    WindOn->setPalette(palette);

    if (myRideItem)
        refresh(myRideItem);
}

void
NotioCDAWindow::rideSelected() 
{
    RideItem *wRideItem = myRideItem;
    RideFile *ride = nullptr;

    if (wRideItem)
        ride = wRideItem->ride();

    // Show a blank chart when no Notio data.
    m_isBlank = true;
    if (ride)
    {
        m_isBlank = (ride->xdata("BCVX") == nullptr) || (ride->xdata("CDAData") == nullptr);
    }

    // Set blank page.
    setIsBlank(m_isBlank);
    setParamChanged(false);

    // Do nothing is the chart is not visible or there is no ride selected.
    if ((ride == nullptr) || !amVisible()) return;

    QString wMsgDesc = tr("We need to compute data to analyse CdA.");

    // Get current chart pointer.
    GcChartWindow *wCurrentChart = context->tab->view(context->viewIndex)->page()->currentChart();
    bool wNeedToRecompute = (ride->xdata("CDAData") && (ride->getTag("GC Version", "0").toInt() < NK_MIN_VERSION)) ||
            (ride->xdata("CDAData") && (ride->getTag("GC Min Version", "0").toInt() > NK_VERSION_LATEST));

    // Check if chart is active window and selected.
    if (isActiveWindow() && (wCurrentChart == this) && m_msgBox)
    {
        bool wShowMsgBox = false;
        m_msgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        m_msgBox->setButtonText(QMessageBox::Ok, tr("Compute"));

        // Check if old Notio data format.
        if (m_isBlank && ride->xdata("ROW"))
        {
            // Show a message to compute data before proceeding.
            m_msgBox->setText(wMsgDesc.prepend(tr("Old data format.")));
            wShowMsgBox = true;
        }
        // Check if Notio data hasn't been computed yet.
        else if (ride->xdata("BCVX") && (ride->xdata("CDAData") == nullptr))
        {
            // Show a message to compute data before proceeding.
            m_msgBox->setText(wMsgDesc.prepend(tr("No CDAData available. ")));
            QString wDummyMsg;
            wShowMsgBox = NotioFuncCompute::speedPowerUnavailable(ride, wDummyMsg) == false;
        }
        // Check if the ride is computed but with a different version of GC.
        else if (wNeedToRecompute)
        {
            // Show a message to recompute data before proceeding.
            m_msgBox->setText(tr("Your ride has been computed with another GC Notio version."));
            m_msgBox->setInformativeText(tr("You should recompute to avoid any data inconsistencies."));
            m_msgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Ignore);
            m_msgBox->setButtonText(QMessageBox::Ok, tr("Recompute"));

            QString wDummyMsg;
            wShowMsgBox = NotioFuncCompute::speedPowerUnavailable(ride, wDummyMsg) == false;
        }

        // Show message box to the user.
        if (wShowMsgBox)
        {
            if (m_msgBox->exec() == QMessageBox::Ok)
            {
                // Compute ride data.
                NotioFuncCompute compute(wRideItem);
                compute();
            }
        }
    }

    // Do nothing if there is not data.
    if (m_isBlank) return;

    // Set newly selected ride item to the Notio CdA plot.
    aerolab->rideSelected(wRideItem);

    loadRideParameters();

    refresh(wRideItem, true);

    allZoomer->setZoomBase();
}

void
NotioCDAWindow::setCrrFromValue(double iValue)
{
    int value = static_cast<int>(1000000 * iValue);

    if (aerolab->intCrr() != value) {
        setParamChanged();
        aerolab->setIntCrr(value);
        crrSlider->setValue(aerolab->intCrr());
        RideItem *ride = context->rideItem();
        refresh(ride);
    }
}

void
NotioCDAWindow::setCrrFromSlider()
{
    if (aerolab->intCrr() != crrSlider->value()) {
        setParamChanged();
        aerolab->setIntCrr(crrSlider->value());
        crrDoubleSpinBox->setValue(aerolab->getCrr());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCdaFromValue(double iValue)
{
    int value = static_cast<int>(10000 * iValue);
    if (aerolab->intCda() != value)
    {
        // Only set save button enable; no need to compute ride.
        btnSave->setEnabled(true);

        aerolab->setIntCda(value);
        aerolab->setCdaOffsetMarker(iValue);
        cdaSlider->setValue(aerolab->intCda());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}
void
NotioCDAWindow::setCdaRangeFromValue(double iValue)
{
    int value = static_cast<int>(iValue * 10000);
    cout << "Set CdaRange from text " << value << endl;
    if (aerolab->intCdaRange() != value)
    {
        // Only set save button enable; no need to compute ride.
        btnSave->setEnabled(true);

        aerolab->setIntCdaRange(value);
        cdaRangeSlider->setValue(aerolab->intCdaRange());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCdaFromSlider()
{
    if (aerolab->intCda() != cdaSlider->value())
    {
        // Only set save button enable; no need to compute ride.
        btnSave->setEnabled(true);

        aerolab->setIntCda(cdaSlider->value());
        cdaDoubleSpinBox->setValue(aerolab->getCda());
        aerolab->setCdaOffsetMarker(aerolab->getCda());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCdaRangeFromSlider()
{
    cout << "Set CdaRange from slider " <<  endl;

    if (aerolab->intCdaRange() != cdaRangeSlider->value())
    {
        // Only set save button enable; no need to compute ride.
        btnSave->setEnabled(true);

        aerolab->setIntCdaRange(cdaRangeSlider->value());
        cdaRangeDoubleSpinBox->setValue(aerolab->getCdaRange());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setTotalMassFromValue(double iValue)
{
    int value = static_cast<int>(100 * iValue);

    if (aerolab->intTotalMass() != value) {
        setParamChanged();
        aerolab->setIntTotalMass(value);
        mSlider->setValue(aerolab->intTotalMass());
        RideItem *ride = context->rideItem();
        refresh(ride);
    }
}

void
NotioCDAWindow::setTotalMassFromSlider()
{
    if (aerolab->intTotalMass() != mSlider->value()) {
        setParamChanged();
        aerolab->setIntTotalMass(mSlider->value());
        mMassDoubleSpinBox->setValue(aerolab->getTotalMass());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setFactorFromValue(double iValue)
{
    int value = static_cast<int>(10000 * iValue);

    if (aerolab->intRiderFactor() != value) {
        setParamChanged();
        cout << "in SetFactorFromText " << value << endl;
        cout << "in SetFactorFromText " << QString("%1").arg(iValue).toStdString() << endl;
        cout << "call setIntRiderFactor " << value << endl;
        aerolab->setIntRiderFactor(value);
        factorSlider->setValue(aerolab->intRiderFactor());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setFactorFromSlider()
{
    if (aerolab->intRiderFactor() != factorSlider->value()) {
        setParamChanged();
        cout << "in setFactorFromSlider " << endl;
        cout << "call setIntRiderFactor " << endl;
        aerolab->setIntRiderFactor(factorSlider->value());
        factorDoubleSpinBox->setValue(aerolab->getRiderFactor());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}
void
NotioCDAWindow::setExponentFromValue(double iValue)
{
    int value = static_cast<int>(10000 * iValue);
    if (aerolab->intRiderExponent() != value) {
        setParamChanged();
        cout << "in setExponentFromText " << value << endl;
        cout << "call setIntRiderExponent " << value << endl;
        aerolab->setIntRiderExponent(value);
        exponentSlider->setValue(aerolab->intRiderExponent());
        RideItem *ride = myRideItem;
        refresh(ride);
    }    
}

void
NotioCDAWindow::setExponentFromSlider()
{
    if (aerolab->intRiderExponent() != exponentSlider->value()) {
        setParamChanged();
        cout << "in setExponentFromSlider" << endl;
        cout << "call setIntRiderExponent" << endl;
        aerolab->setIntRiderExponent(exponentSlider->value());
        exponentDoubleSpinBox->setValue(aerolab->getRiderExponent());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setEtaFromValue(double iValue)
{
    int value = static_cast<int>(10000 * iValue);
    if (aerolab->intEta() != value) {
        setParamChanged();
        aerolab->setIntEta(value);
        etaSlider->setValue(aerolab->intEta());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::setComment
///        This method is called upon Comment field modification.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::setComment(const QString)
{
    // Enable save parameters button; no need to compute ride.
    btnSave->setEnabled(true);
}

void
NotioCDAWindow::setEtaFromSlider()
{
    if (aerolab->intEta() != etaSlider->value()) {
        setParamChanged();
        aerolab->setIntEta(etaSlider->value());
        etaDoubleSpinBox->setValue(aerolab->getEta());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCalcWindowFromValue(double iValue)
{
    if (aerolab->intCalcWindow() != iValue) {
        setParamChanged();
        aerolab->setIntCalcWindow(iValue);
        calcWindowSlider->setValue(aerolab->intCalcWindow());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCDAWindowFromValue(double iValue)
{
    if (aerolab->intCDAWindow() != iValue) {
        setParamChanged();
        aerolab->setIntCDAWindow(iValue);
        cdaWindowSlider->setValue(aerolab->intCDAWindow());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setTimeOffsetFromValue(double iValue)
{
    if (aerolab->intTimeOffset() != iValue) {
        setParamChanged();
        aerolab->setIntTimeOffset(iValue);
        timeOffsetSlider->setValue(aerolab->intTimeOffset());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setInertiaFactorFromValue(double iValue)
{
    int value = static_cast<int>(10000 * iValue);
    if (aerolab->intInertiaFactor() != value) {
        setParamChanged();
        aerolab->setIntInertiaFactor(value);
        inertiaFactorSlider->setValue(aerolab->intInertiaFactor());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCalcWindowFromSlider()
{
    if (aerolab->intCalcWindow() != calcWindowSlider->value()) {
        setParamChanged();
        aerolab->setIntCalcWindow(calcWindowSlider->value());
        calcWindowDoubleSpinBox->setValue(aerolab->getCalcWindow());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setCDAWindowFromSlider()
{
    if (aerolab->intCDAWindow() != cdaWindowSlider->value()) {
        setParamChanged();
        aerolab->setIntCDAWindow(cdaWindowSlider->value());
        cdaWindowDoubleSpinBox->setValue(aerolab->getCDAWindow());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setTimeOffsetFromSlider()
{
    if (aerolab->intTimeOffset() != timeOffsetSlider->value()) {
        setParamChanged();
        aerolab->setIntTimeOffset(timeOffsetSlider->value());
        timeOffsetDoubleSpinBox->setValue(aerolab->getTimeOffset());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setInertiaFactorFromSlider()
{
    if (aerolab->intInertiaFactor() != inertiaFactorSlider->value()) {
        setParamChanged();
        aerolab->setIntInertiaFactor(inertiaFactorSlider->value());
        inertiaFactorDoubleSpinBox->setValue(aerolab->getInertiaFactor());
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

void
NotioCDAWindow::setGarminON(int value)
{
    aerolab->setGarminON(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride);
}

////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::setWindOn
///        This method is called when the wind checkbox state change.
///
/// \param[in] value Wind checkbox state.
////////////////////////////////////////////////////////////////////////////
void
NotioCDAWindow::setWindOn(int value)
{
    WindOn->setChecked(static_cast<bool>(value));

    aerolab->setWindOn(value);

    // refresh
    if (myRideItem)
    {
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::setByDistance
///        This method is called when selecting the X axis mode.
///
/// \param[in] value State of the combo box.
////////////////////////////////////////////////////////////////////////////
void
NotioCDAWindow::setByDistance(int value)
{
    comboDistance->setCurrentIndex(value);

    aerolab->setByDistance(value);

    // Reset zoom stack.
    allZoomer->setZoomBase();

    // refresh
    if (myRideItem)
    {
        RideItem *ride = myRideItem;
        refresh(ride);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::isByDistance
///        This method returns the state of the combo box use to select the X
///        axis mode.
///
/// \return The X axis combo box state.
///////////////////////////////////////////////////////////////////////////////
int NotioCDAWindow::isByDistance()
{
    return static_cast<int>(aerolab->byDistance());
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::doEstFactExp
///        This methods estimates the coefficient 1.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::doEstCalibFact()
{
    RideItem *ride = context->rideItem();

    if (ride)
    {
        // Estimate Calibration coefficient.
        double wFactor = 0.0, wExponent = aerolab->getRiderExponent();
        const QString errMsg = aerolab->estimateFactExp(ride, wExponent, wFactor);

        // Create confirmation dialog box depending on the results.
        QMessageBox wConfirm(nullptr);
        wConfirm.setWindowTitle(tr("Notio CdA Analysis"));
        wConfirm.setText(errMsg.isEmpty() ? QString(tr("Your estimated calibration factor is %1.")).arg(wFactor) : tr("Cannot estimate calibration factor."));
        wConfirm.setInformativeText(errMsg.isEmpty() ? "" : errMsg);
        wConfirm.setIcon(errMsg.isEmpty() ? QMessageBox::Information : QMessageBox::Warning);
        wConfirm.setStandardButtons(errMsg.isEmpty() ? (QMessageBox::Apply | QMessageBox::Ignore) : QMessageBox::Ok);

        // Backup current calibration factor
        double wCurrentValue = aerolab->getRiderFactor();

        // Calibration factor estimation valid.
        if (errMsg.isEmpty())
        {
            // Preview wind and CdA on the graph.
            int value = static_cast<int>(wFactor * 10000);
            aerolab->setIntRiderFactor(value);
            refresh(ride);
        }

        int wDialogResult = wConfirm.exec();

        // User accept the estimated coefficient.
        if (wDialogResult & QMessageBox::Apply)
        {
            // Update calibration factor.
            int value = static_cast<int>(wFactor * 10000);
            aerolab->setIntRiderFactor(value);
            factorDoubleSpinBox->setValue(wFactor);

            // Save parameters and compute data to apply the changes.
            saveParametersInRide();
        }
        // Restore graph as before preview.
        else
        {
            aerolab->setIntRiderFactor(static_cast<int>(wCurrentValue * 10000));
            refresh(ride);
        }
    }
}

void
NotioCDAWindow::doEstCdACrr()
{
    RideItem *ride = context->rideItem();
    /* Estimate Crr&Cda */
    const QString errMsg = aerolab->estimateCdACrr(ride);
    if (errMsg.isEmpty()) {
        /* Update Crr/Cda values values in UI */
        crrDoubleSpinBox->setValue(aerolab->getCrr());
        crrSlider->setValue(aerolab->intCrr());
        cdaDoubleSpinBox->setValue(aerolab->getCda());
        cdaSlider->setValue(aerolab->intCda());
        /* Refresh */
        refresh(ride);
    } else {
        /* report error: insufficient data to estimate Cda&Crr */
        QMessageBox::warning(this, tr("Estimate CdA and Crr"), errMsg);
    }
}


void
NotioCDAWindow::zoomInterval(IntervalItem *which) {
    QwtDoubleRect rect;

    double wMultiplier = (context->athlete->useMetricUnits ? 1 : static_cast<double>(MILES_PER_KM));
    if (!aerolab->byDistance()) {
        rect.setLeft(which->start / 60);
        rect.setRight(which->stop / 60);
    } else {
        rect.setLeft(which->startKM * wMultiplier);
        rect.setRight(which->stopKM * wMultiplier);
    }
    //NKC1
    allZoomer->zoom(rect);

    aerolab->recalc(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::zoomOut
///        This methods zooms out the plot.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::zoomOut()
{
    // RideItem validation
    if (myRideItem)
    {
        // Refresh plot with new zoom.
        RideItem *ride = myRideItem;
        refresh(ride, true);

        // Clear zoom stack.
        allZoomer->setZoomBase();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::intervalSelected
///        This method is called upon interval selection.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::intervalSelected()
{
    RideItem *ride = myRideItem;
    if (amVisible() && ride && ride->ride())
    {
        double wConversion = (context->athlete->useMetricUnits ? 1 : static_cast<double>(MILES_PER_KM));
        bool wNewZoom = false;

        // Verify the selected intervals are outside the current view (zoomed window).
        for (auto &wIntervalItr : ride->intervalsSelected())
        {
            // Get interval start and stop depending on the X axis units.
            double wStart = aerolab->byDistance() ? wIntervalItr->startKM * wConversion : wIntervalItr->start / 60;
            double wStop = aerolab->byDistance() ? wIntervalItr->stopKM * wConversion : wIntervalItr->stop / 60;

            // Get Plot Zoomer dimensions.
            QRectF wCurrentView = allZoomer->zoomRect();

            // Interval is outside the current view.
            if (wCurrentView.left() > wStart || wCurrentView.right() < wStop)
            {
                // Zoom need to be reset.
                wNewZoom = true;
                break;
            }
        }

        // Refresh selection.
        refresh(ride, wNewZoom);
    }
}

double NotioCDAWindow::getCanvasTop() const
{
    const QwtDoubleRect &canvasRect = allZoomer->zoomRect();
    return canvasRect.top();
}

double NotioCDAWindow::getCanvasBottom() const
{
    const QwtDoubleRect &canvasRect = allZoomer->zoomRect();
    return canvasRect.bottom();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::isWindOn
///        This method returns the wind checkbox state.
///
/// \return The wind checkbox state.
///////////////////////////////////////////////////////////////////////////////
bool NotioCDAWindow::isWindOn()
{
    return WindOn->isChecked();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::saveParametersInRide
///        This method saves the ride parameters into the ride metadata.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::saveParametersInRide()
{
    if (rideItem() && rideItem()->ride())
    {
        rideItem()->ride()->setTag("notio.targetCDA", QString("%1").arg(aerolab->getCda()));            // Chart CdA Offset
        rideItem()->ride()->setTag("notio.cdaRange", QString("%1").arg(aerolab->getCdaRange()));        // Chart CdA Range
        rideItem()->ride()->setTag("customCrr", QString("%1").arg(aerolab->getCrr()));                  // Custom CRR
        rideItem()->ride()->setTag("customWeight", QString("%1").arg(aerolab->getTotalMass()));         // Custom Total Weight
        rideItem()->ride()->setTag("customMecEff", QString("%1").arg(aerolab->getEta()));               // Custom Efficiency
        rideItem()->ride()->setTag("customInertia", QString("%1").arg(aerolab->getInertiaFactor()));    // Custom Inertia
        rideItem()->ride()->setTag("customRiderFactor", QString("%1").arg(aerolab->getRiderFactor()));  // Custom Rider Factor
        rideItem()->ride()->setTag("customExponent", QString("%1").arg(aerolab->getRiderExponent()));   // Custom Rider Exponent
        rideItem()->ride()->setTag("notio.calcWindow", QString("%1").arg(aerolab->getCalcWindow()));
        rideItem()->ride()->setTag("notio.cdaWindow", QString("%1").arg(aerolab->getCDAWindow()));      // CDAData CdA Ekf calculation
        rideItem()->ride()->setTag("aerolab.Comment", QString("%1").arg(commentEdit->text()));

        // For compatibility issues, rides computed with this version will be compatible with previous GC version.
        // To be removed in future release.
        rideItem()->ride()->setTag("notio.crr", QString("%1").arg(aerolab->getCrr()));
        rideItem()->ride()->setTag("notio.riderWeight", QString("%1").arg(aerolab->getTotalMass()));
        rideItem()->ride()->setTag("notio.efficiency", QString("%1").arg(aerolab->getEta()));
        rideItem()->ride()->setTag("notio.inertiaFactor", QString("%1").arg(aerolab->getInertiaFactor()));
        rideItem()->ride()->setTag("notio.riderFactor", QString("%1").arg(aerolab->getRiderFactor()));
        rideItem()->ride()->setTag("notio.riderExponent", QString("%1").arg(aerolab->getRiderExponent()));
        // ==================================

        // Check if main parameters changed.
        if (m_mainParamChanged)
        {
            // Regenerates the CDAData series with the new parameters.
            NotioFuncCompute compute(rideItem());
            compute();
        }
    }

    // Disable save button and reset parameters changed flag.
    setParamChanged(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::refresh
///        This method refreshes the plot.
///
/// \param[in] iRideItem    Current RideItem.
/// \param[in] iNewZoom     Zoom flag for resetting the zoom.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::refresh(RideItem *iRideItem, bool iNewZoom)
{
    if (iRideItem)
    {
        qDebug() << "Got a refresh" << (iNewZoom ? "true" : "false");
        aerolab->setData(iRideItem, iNewZoom);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::loadRideParameters
///        This method loads CdA Analysis Chart parameters from the ride file.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::loadRideParameters()
{
    if (rideItem())
    {
        RideFile *wRide = rideItem()->ride();
        if (wRide == nullptr)
            return;

        // Load CdA Scale parameters.
        // Load CdA scale offset.
        double wCda = wRide->getTag("notio.targetCDA", "0").toDouble();
        if (wCda > 0)
        {
            int wValue = static_cast<int>(10000 * wCda);
            aerolab->setIntCda(wValue);
            aerolab->setCdaOffsetMarker(wCda);
            cdaDoubleSpinBox->setValue(aerolab->getCda());
            cdaSlider->setValue(aerolab->intCda());
        }

        // Load CdA scale range.
        double wCdaRange = wRide->getTag("notio.cdaRange", "0").toDouble();
        if (wCdaRange > 0)
        {
            int wValue = static_cast<int>(10000 * wCdaRange);
            aerolab->setIntCdaRange(wValue);
            cdaRangeDoubleSpinBox->setValue(aerolab->getCdaRange());
            cdaRangeSlider->setValue(aerolab->intCdaRange());
        }

        // Load Comment.
        QString wComment = wRide->getTag("aerolab.Comment", "");
        if (wComment.isEmpty() == false)
        {
            commentEdit->setText(wComment);
        }

        // Load main parameters that will impact computation.
        // Load rider weight.
        double wTotalMass = wRide->getTag("customWeight", wRide->getTag("notio.riderWeight", "80")).toDouble();
        if (wTotalMass > 0)
        {
            int wValue = static_cast<int>(100 * wTotalMass);
            aerolab->setIntTotalMass(wValue);
            mMassDoubleSpinBox->setValue(aerolab->getTotalMass());
            mSlider->setValue(aerolab->intTotalMass());
        }

        // Load rolling resistance.
        double wCrr = wRide->getTag("customCrr", wRide->getTag("notio.crr", "0.004")).toDouble();
        if (wCrr > 0)
        {
            int wValue = static_cast<int>(1000000 * wCrr);
            aerolab->setIntCrr(wValue);
            crrDoubleSpinBox->setValue(aerolab->getCrr());
            crrSlider->setValue(aerolab->intCrr());
        }

        // Load efficiency parameter.
        double wEta = wRide->getTag("customMecEff", wRide->getTag("notio.efficiency", "1.0")).toDouble();
        if (wEta > 0)
        {
            int wValue = static_cast<int>(10000 * wEta);
            aerolab->setIntEta(wValue);
            etaDoubleSpinBox->setValue(aerolab->getEta());
            etaSlider->setValue(aerolab->intEta());
        }

        // Load Coefficient 1.
        double wRiderFactor = wRide->getTag("customRiderFactor", wRide->getTag("notio.riderFactor", "1.39")).toDouble();
        if (wRiderFactor > 0)
        {
            int wValue = static_cast<int>(10000 * wRiderFactor);
            aerolab->setIntRiderFactor(wValue);
            factorDoubleSpinBox->setValue(aerolab->getRiderFactor());
            factorSlider->setValue(aerolab->intRiderFactor());
        }

        // Load stick factor. (should be -0.05 and it is not shown to the user)
        double wRiderExponent = wRide->getTag("customExponent", wRide->getTag("notio.riderExponent", "-0.05")).toDouble();
        cout << "RiderExponent" << wRiderExponent << endl;
        if (wRiderExponent > -9999.0)
        {
            int wValue = static_cast<int>(10000 * wRiderExponent);
            aerolab->setIntRiderExponent(wValue);
            exponentDoubleSpinBox->setValue(aerolab->getRiderExponent());
            exponentSlider->setValue(aerolab->intRiderExponent());
        }

        // Load legacy parameters (not displayed to the user).
        double wCalcWindow  = wRide->getTag("notio.calcWindow", "60").toDouble();
        if (wCalcWindow > 0)
        {
            aerolab->setIntCalcWindow(static_cast<int>(wCalcWindow));
            calcWindowDoubleSpinBox->setValue(aerolab->getCalcWindow());
            calcWindowSlider->setValue(static_cast<int>(aerolab->getCalcWindow()));
        }

        double wCdaWindow  = wRide->getTag("notio.cdaWindow", "60").toDouble();
        if (wCdaWindow > 0)
        {
            aerolab->setIntCDAWindow(static_cast<int>(wCdaWindow));
            cdaWindowDoubleSpinBox->setValue(aerolab->getCDAWindow());
            cdaWindowSlider->setValue(static_cast<int>(aerolab->getCDAWindow()));
        }

        double wTimeOffset = wRide->getTag("notio.timeOffset", "0").toDouble();
        if (true/*wTimeOffset > 0*/)
        {
            aerolab->setIntTimeOffset(static_cast<int>(wTimeOffset));
            timeOffsetDoubleSpinBox->setValue(aerolab->getTimeOffset());
            timeOffsetSlider->setValue(static_cast<int>(aerolab->getTimeOffset()));
        }

        double wInertiaFactor = wRide->getTag("customInertia", wRide->getTag("notio.inertiaFactor", "1.0")).toDouble();
        if (true/*wInertiaFactor > 0*/)
        {
            int wValue = static_cast<int>(wInertiaFactor * 10000);
            aerolab->setIntInertiaFactor(wValue);
            double wFactor = aerolab->getInertiaFactor();
            cout << "inertia factor " << wFactor << endl;
            inertiaFactorDoubleSpinBox->setValue(wFactor);
            inertiaFactorSlider->setValue(aerolab->intInertiaFactor());
        }

        // No main parameters have changed; only loaded.
        setParamChanged(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCDAWindow::setParamChanged
///        This method enables the Save Parameters button and indicates that a
///        main parameter has been changed.
///
/// \param[in] iEnable  State of the button.
///////////////////////////////////////////////////////////////////////////////
void NotioCDAWindow::setParamChanged(bool iEnable)
{
    m_mainParamChanged = iEnable;
    btnSave->setEnabled(iEnable);
}
