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
#include "Context.h"
#include "NotioVEWindow.h"
#include "NotioVE.h"
#include "TabView.h"
#include "IntervalItem.h"
#include "RideItem.h"
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <QtGui>
#include <qwt_plot_zoomer.h>
#include <QListWidget>

#include <iostream>
using namespace std;

NotioVEWindow::NotioVEWindow(Context *context) :
    GcChartWindow(context), context(context) {
    setControls(NULL);

    // NotioVE tab layout:
    QVBoxLayout *vLayout      = new QVBoxLayout;
    QHBoxLayout *cLayout      = new QHBoxLayout;

    // Plot:
    aerolab = new NotioVE(this, context);
    newZoom = false; // this is to fix bug of infinite loop on selected interval

    HelpWhatsThis *help = new HelpWhatsThis(aerolab);
    aerolab->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Aerolab));

    // Left controls layout:
    QVBoxLayout *leftControls  =  new QVBoxLayout;
    QFontMetrics metrics(QApplication::font());
    int labelWidth1 = metrics.width("Intervals") + 10;

    // Crr:
    QHBoxLayout *crrLayout = new QHBoxLayout;
    crrLabel = new QLabel(tr("Crr"), this);
    crrLabel->setFixedWidth(labelWidth1);
    crrLineEdit = new QLineEdit();
    crrLineEdit->setFixedWidth(75);
    crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
    /*crrQLCDNumber    = new QLCDNumber(7);
  crrQLCDNumber->setMode(QLCDNumber::Dec);
  crrQLCDNumber->setSmallDecimalPoint(false);
  crrQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()) );*/
    crrSlider = new QSlider(Qt::Horizontal);
    crrSlider->setTickPosition(QSlider::TicksBelow);
    crrSlider->setTickInterval(1000);
    crrSlider->setMinimum(1000);
    crrSlider->setMaximum(15000);
    crrSlider->setValue(aerolab->intCrr());
    crrLayout->addWidget( crrLabel );
    crrLayout->addWidget( crrLineEdit );
    //crrLayout->addWidget( crrQLCDNumber );
    crrLayout->addWidget( crrSlider );

    // CdA:
    QHBoxLayout *cdaLayout = new QHBoxLayout;
    cdaLabel = new QLabel(tr("CdA"), this);
    cdaLabel->setFixedWidth(labelWidth1);
    cdaLineEdit = new QLineEdit();
    cdaLineEdit->setFixedWidth(75);
    cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
    /*cdaQLCDNumber    = new QLCDNumber(7);
  cdaQLCDNumber->setMode(QLCDNumber::Dec);
  cdaQLCDNumber->setSmallDecimalPoint(false);
  cdaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()) );*/
    cdaSlider = new QSlider(Qt::Horizontal);
    cdaSlider->setTickPosition(QSlider::TicksBelow);
    cdaSlider->setTickInterval(100);
    cdaSlider->setMinimum(1);
    cdaSlider->setMaximum(6000);
    cdaSlider->setValue(aerolab->intCda());
    cdaLayout->addWidget( cdaLabel );
    //cdaLayout->addWidget( cdaQLCDNumber );
    cdaLayout->addWidget( cdaLineEdit );
    cdaLayout->addWidget( cdaSlider );

    // Eta:
    QHBoxLayout *etaLayout = new QHBoxLayout;
    etaLabel = new QLabel(tr("Eta"), this);
    etaLabel->setFixedWidth(labelWidth1);
    etaLineEdit = new QLineEdit();
    etaLineEdit->setFixedWidth(75);
    etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
    /*etaQLCDNumber    = new QLCDNumber(7);
  etaQLCDNumber->setMode(QLCDNumber::Dec);
  etaQLCDNumber->setSmallDecimalPoint(false);
  etaQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()) );*/
    etaSlider = new QSlider(Qt::Horizontal);
    etaSlider->setTickPosition(QSlider::TicksBelow);
    etaSlider->setTickInterval(1000);
    etaSlider->setMinimum(8000);
    etaSlider->setMaximum(12000);
    etaSlider->setValue(aerolab->intEta());
    etaLayout->addWidget( etaLabel );
    etaLayout->addWidget( etaLineEdit );
    //etaLayout->addWidget( etaQLCDNumber );
    etaLayout->addWidget( etaSlider );

    // Eta:
    QHBoxLayout *commentLayout = new QHBoxLayout;

    QComboBox *comboDistance = new QComboBox();
    comboDistance->addItem(tr("X Axis Shows Time"));
    comboDistance->addItem(tr("X Axis Shows Distance"));
    comboDistance->setCurrentIndex(1);
    commentLayout->addWidget(comboDistance);

    QPushButton *syncSelectedLap = new QPushButton(tr("&Sync selection lap"), this);
    commentLayout->addWidget(syncSelectedLap);

    // Add to leftControls:
    leftControls->addLayout( crrLayout );
    leftControls->addLayout( cdaLayout );
    leftControls->addLayout( etaLayout );
    leftControls->addLayout( commentLayout );

    // Right controls layout:
    QVBoxLayout *rightControls  =  new QVBoxLayout;
    int labelWidth2 = metrics.width("Total Mass (kg)") + 10;

    // Total mass:
    QHBoxLayout *mLayout = new QHBoxLayout;
    mLabel = new QLabel(tr("Total Mass (kg)"), this);
    mLabel->setFixedWidth(labelWidth2);
    mLineEdit = new QLineEdit();
    mLineEdit->setFixedWidth(70);
    mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
    /*mQLCDNumber    = new QLCDNumber(7);
  mQLCDNumber->setMode(QLCDNumber::Dec);
  mQLCDNumber->setSmallDecimalPoint(false);
  mQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()) );*/
    mSlider = new QSlider(Qt::Horizontal);
    mSlider->setTickPosition(QSlider::TicksBelow);
    mSlider->setTickInterval(1000);
    mSlider->setMinimum(3500);
    mSlider->setMaximum(15000);
    mSlider->setValue(aerolab->intTotalMass());
    mLayout->addWidget( mLabel );
    mLayout->addWidget( mLineEdit );
    //mLayout->addWidget( mQLCDNumber );
    mLayout->addWidget( mSlider );

    QHBoxLayout *factorLayout = new QHBoxLayout;
    factorLabel = new QLabel(tr("Factor"), this);
    factorLabel->setFixedWidth(labelWidth2);
    factorLineEdit = new QLineEdit();
    factorLineEdit->setFixedWidth(70);
    factorLineEdit->setText(QString("%1").arg(aerolab->getRiderFactor()) );
    /*rhoQLCDNumber    = new QLCDNumber(7);
  rhoQLCDNumber->setMode(QLCDNumber::Dec);
  rhoQLCDNumber->setSmallDecimalPoint(false);
  rhoQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  rhoQLCDNumber->display(QString("%1").arg(aerolab->getRiderFactor()) );*/
    factorSlider = new QSlider(Qt::Horizontal);
    factorSlider->setTickPosition(QSlider::TicksBelow);
    factorSlider->setTickInterval(1000);
    factorSlider->setMinimum(000);
    factorSlider->setMaximum(20000);
    factorSlider->setValue(aerolab->intRiderFactor());
    factorLayout->addWidget( factorLabel );
    factorLayout->addWidget( factorLineEdit );
    //rhoLayout->addWidget( rhoQLCDNumber );
    factorLayout->addWidget( factorSlider );

    QHBoxLayout *exponentLayout = new QHBoxLayout;
    exponentLabel = new QLabel(tr("Exponent)"), this);
    exponentLabel->setFixedWidth(labelWidth2);
    exponentLineEdit = new QLineEdit();
    exponentLineEdit->setFixedWidth(70);
    exponentLineEdit->setText(QString("%1").arg(aerolab->getRiderExponent()) );
    exponentSlider = new QSlider(Qt::Horizontal);
    exponentSlider->setTickPosition(QSlider::TicksBelow);
    exponentSlider->setTickInterval(1000);
    exponentSlider->setMinimum(-10000);
    exponentSlider->setMaximum(10000);
    exponentSlider->setValue(aerolab->intRiderExponent());
    exponentLayout->addWidget( exponentLabel );
    exponentLayout->addWidget( exponentLineEdit );
    //rhoLayout->addWidget( rhoQLCDNumber );
    exponentLayout->addWidget( exponentSlider );

    // Elevation offset:
    QHBoxLayout *eoffsetLayout = new QHBoxLayout;
    eoffsetLabel = new QLabel(tr("Eoffset (m)"), this);
    eoffsetLabel->setFixedWidth(labelWidth2);
    eoffsetLineEdit = new QLineEdit();
    eoffsetLineEdit->setFixedWidth(70);
    eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
    /*eoffsetQLCDNumber    = new QLCDNumber(7);
  eoffsetQLCDNumber->setMode(QLCDNumber::Dec);
  eoffsetQLCDNumber->setSmallDecimalPoint(false);
  eoffsetQLCDNumber->setSegmentStyle(QLCDNumber::Flat);
  eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()) );*/
    eoffsetSlider = new QSlider(Qt::Horizontal);
    eoffsetSlider->setTickPosition(QSlider::TicksBelow);
    eoffsetSlider->setTickInterval(20000);
    eoffsetSlider->setMinimum(-30000);
    eoffsetSlider->setMaximum(250000);
    eoffsetSlider->setValue(aerolab->intEoffset());
    eoffsetLayout->addWidget( eoffsetLabel );
    eoffsetLayout->addWidget( eoffsetLineEdit );
    //eoffsetLayout->addWidget( eoffsetQLCDNumber );
    eoffsetLayout->addWidget( eoffsetSlider );


    QHBoxLayout *smoothLayout = new QHBoxLayout;

    QPushButton *btnEstCdACrr = new QPushButton(tr("&Estimate CdA and Crr"), this);
    smoothLayout->addWidget(btnEstCdACrr);

    btnSave = new QPushButton(tr("&Save parameters"), this);
    smoothLayout->addWidget(btnSave);

    commentLabel = new QLabel(tr("Comment"), this);
    commentLabel->setFixedWidth(labelWidth1);
    commentEdit = new QLineEdit();

    smoothLayout->addWidget( commentLabel );
    smoothLayout->addWidget( commentEdit );

    // Add to leftControls:
    rightControls->addLayout( mLayout );
    rightControls->addLayout( factorLayout );
    rightControls->addLayout( exponentLayout );
    rightControls->addLayout( eoffsetLayout );
    rightControls->addLayout( smoothLayout );

    // Now the checkboxes

    QVBoxLayout *checkboxLayout = new QVBoxLayout;
    eoffsetAuto = new QCheckBox(tr("eoffset auto"), this);
    eoffsetAuto->setCheckState(Qt::Unchecked);
    checkboxLayout->addWidget(eoffsetAuto);

    GarminON  = new QCheckBox(tr("Garmin altitude"), this);
    GarminON->setCheckState(Qt::Checked);
    checkboxLayout->addWidget(GarminON);

    WindOn  = new QCheckBox(tr("Wind"), this);
    WindOn->setCheckState(Qt::Checked);
    checkboxLayout->addWidget(WindOn);

    constantAlt = new QCheckBox(tr("Constant altitude (velodrome,...)"), this);
    checkboxLayout->addWidget(constantAlt);

    //MGG  eoffsetLayout->addLayout(checkboxLayout);

    // Assemble controls layout:
    cLayout->addLayout(leftControls);
    cLayout->addLayout(rightControls);
    cLayout->addLayout(checkboxLayout); //MGG

    // Zoomer:
    allZoomer = new QwtPlotZoomer(aerolab->canvas());
    allZoomer->setRubberBand(QwtPicker::RectRubberBand);
    allZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    allZoomer->setEnabled(true);
    allZoomer->setMousePattern( QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier );
    allZoomer->setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton );

    // SIGNALs to SLOTs:
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(rideChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(crrSlider, SIGNAL(valueChanged(int)),this, SLOT(setCrrFromSlider()));
    connect(crrLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setCrrFromText(const QString)));
    connect(cdaSlider, SIGNAL(valueChanged(int)), this, SLOT(setCdaFromSlider()));
    connect(cdaLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setCdaFromText(const QString)));
    connect(mSlider, SIGNAL(valueChanged(int)),this, SLOT(setTotalMassFromSlider()));
    connect(mLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setTotalMassFromText(const QString)));
    connect(factorSlider, SIGNAL(valueChanged(int)), this, SLOT(setFactorFromSlider()));
    connect(factorLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setFactorFromText(const QString)));
    connect(exponentSlider, SIGNAL(valueChanged(int)), this, SLOT(setExponentFromSlider()));
    connect(exponentLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setExponentFromText(const QString)));
    connect(etaSlider, SIGNAL(valueChanged(int)), this, SLOT(setEtaFromSlider()));
    connect(etaLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setEtaFromText(const QString)));
    connect(commentEdit, SIGNAL(textChanged(const QString)), this, SLOT(setComment(const QString)));
    connect(eoffsetSlider, SIGNAL(valueChanged(int)), this, SLOT(setEoffsetFromSlider()));
    connect(eoffsetLineEdit, SIGNAL(textChanged(const QString)), this, SLOT(setEoffsetFromText(const QString)));
    connect(eoffsetAuto, SIGNAL(stateChanged(int)), this, SLOT(setAutoEoffset(int)));
    connect(constantAlt, SIGNAL(stateChanged(int)), this, SLOT(setConstantAlt(int)));
    connect(GarminON, SIGNAL(stateChanged(int)), this, SLOT(setGarminON(int)));
    connect(WindOn, SIGNAL(stateChanged(int)), this, SLOT(setWindOn(int)));
    connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(setByDistance(int)));
    connect(btnEstCdACrr, SIGNAL(clicked()), this, SLOT(doEstCdACrr()));
    connect(btnSave, SIGNAL(clicked()), this, SLOT(saveParametersInRide()));
    connect(context, SIGNAL(configChanged(qint32)), aerolab, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(intervalSelected() ), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalZoom(IntervalItem*) ), this, SLOT(zoomInterval(IntervalItem*)));
    connect(allZoomer, SIGNAL( zoomed(const QRectF) ), this, SLOT(zoomChanged()));


    // Build the tab layout:
    vLayout->addWidget(aerolab);
    vLayout->addLayout(cLayout);
    setChartLayout(vLayout);


    // tooltip on hover over point
    //************************************
    aerolab->tooltip = new LTMToolTip( QwtPlot::xBottom, QwtPlot::yLeft,
                                       QwtPicker::VLineRubberBand, QwtPicker::AlwaysOn,
                                       aerolab->canvas(), "");
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

void
NotioVEWindow::zoomChanged()
{
    // Check if blank.
    if (m_isBlank)
    {
        qDebug() << "Do not refresh if page is blank.";
        return;
    }

    RideItem *ride = myRideItem;
    refresh(ride, false);
}


void
NotioVEWindow::configChanged(qint32)
{
    allZoomer->setRubberBandPen(GColor(CPLOTSELECT));
    setProperty("color", GColor(CPLOTBACKGROUND));

    QPalette palette;

    palette.setColor(QPalette::Window, GColor(CPLOTBACKGROUND));
    palette.setColor(QPalette::Background, GColor(CPLOTBACKGROUND));

    // only change base if moved away from white plots
    // which is a Mac thing
#ifndef Q_OS_MAC
    if (GColor(CPLOTBACKGROUND) != Qt::white)
#endif
    {
        palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
        palette.setColor(QPalette::Window,  GColor(CPLOTBACKGROUND));
    }

    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
    aerolab->setPalette(palette);
    crrLabel->setPalette(palette);
    cdaLabel->setPalette(palette);
    etaLabel->setPalette(palette);
    commentLabel->setPalette(palette);
    mLabel->setPalette(palette);
    factorLabel->setPalette(palette);
    exponentLabel->setPalette(palette);
    eoffsetLabel->setPalette(palette);
    crrSlider->setPalette(palette);
    crrLineEdit->setPalette(palette);
    cdaSlider->setPalette(palette);
    cdaLineEdit->setPalette(palette);
    mSlider->setPalette(palette);
    mLineEdit->setPalette(palette);
    factorSlider->setPalette(palette);
    factorLineEdit->setPalette(palette);
    exponentLineEdit->setPalette(palette);
    etaSlider->setPalette(palette);
    etaLineEdit->setPalette(palette);
    eoffsetLineEdit->setPalette(palette);
    eoffsetAuto->setPalette(palette);
    GarminON->setPalette(palette);
    WindOn->setPalette(palette);
    constantAlt->setPalette(palette);
    commentEdit->setPalette(palette);

#ifndef Q_OS_MAC
    aerolab->setStyleSheet(TabView::ourStyleSheet());
#endif
}

void
NotioVEWindow::rideSelected() {
    RideItem *ride = myRideItem;
    RideFile *wRideFile = nullptr;

    if (ride)
        wRideFile = ride->ride();

    // Show a blank chart when no Notio Konect data.
    m_isBlank = true;
    if (wRideFile)
    {
        m_isBlank = (wRideFile->xdata("BCVX") == nullptr) || (wRideFile->xdata("CDAData") == nullptr);
    }

    // Set blank page.
    setIsBlank(m_isBlank);

    // Do nothing is the chart is not visible or there is no ride selected.
    if ((wRideFile == nullptr) || !amVisible())  return;

    QString wMsgDesc = tr("We need to compute data to analyse CdA.");
    if (m_msgBox == nullptr)
    {
        m_msgBox = new QMessageBox(this);

        m_msgBox->setWindowTitle(tr("Notio VE Analysis"));
        m_msgBox->setStandardButtons(QMessageBox::Ok);
        m_msgBox->setIcon(QMessageBox::Information);
        m_msgBox->setWindowModality(Qt::WindowModality::WindowModal);
    }

    // Check if chart is active window.
    if (isActiveWindow())
    {
        // Check if old Notio Konect data format.
        if (m_isBlank && wRideFile->xdata("ROW"))
        {
            // Show a message to compute data before proceeding.
            m_msgBox->setText(wMsgDesc.prepend(tr("Old data format.")));
            m_msgBox->show();
        }
        // Check if Notio Konect data hasn't been computed yet.
        else if (wRideFile->xdata("BCVX") && (wRideFile->xdata("CDAData") == nullptr))
        {
            // Show a message to compute data before proceeding.
            m_msgBox->setText(wMsgDesc.prepend(tr("No CDAData available.")));
            m_msgBox->show();
        }
    }

    // Do nothing if there is not data.
    if (m_isBlank) return;

    commentEdit->setText("");

    // Read values en Ride
    double totalMass = ride->ride()->getTag("customWeight", ride->ride()->getTag("notio.riderWeight", "0")).toDouble();
    if (totalMass>0) {
        int value = 100 * totalMass;
        aerolab->setIntTotalMass(value);
        mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
        mSlider->setValue(aerolab->intTotalMass());
    }

    if (hasNewParametersInRide()) {
        cout << "Have New values" << endl;
        double eta = ride->ride()->getTag("customMecEff", ride->ride()->getTag("notio.efficiency", "0")).toDouble();
        if (eta>0) {
            int value = 10000 * eta;
            aerolab->setIntEta(value);
            etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
            etaSlider->setValue(aerolab->intEta());
        }

        double riderFactor = ride->ride()->getTag("customRiderFactor",ride->ride()->getTag("notio.riderFactor", "0")).toDouble();
        if (riderFactor>0) {
            int value = 10000 * riderFactor;
            aerolab->setIntRiderFactor(value);
            factorLineEdit->setText(QString("%1").arg(aerolab->getRiderFactor()) );
            factorSlider->setValue(aerolab->intRiderFactor());
        }

        double riderExponent = ride->ride()->getTag("customExponent",ride->ride()->getTag("notio.riderExponent", "0")).toDouble();
        cout << "RiderExponent" << riderExponent << endl;
        if (riderExponent> -9999.0) {
            int value = 10000 * riderExponent;
            aerolab->setIntRiderExponent(value);
            exponentLineEdit->setText(QString("%1").arg(aerolab->getRiderExponent()) );
            exponentSlider->setValue(aerolab->intRiderExponent());
        }

        double eoffset = ride->ride()->getTag("aerolab.EOffset", "0").toDouble();
        if (eoffset!=0) {
            int value = 100 * eoffset;
            aerolab->setIntEoffset(value);
            eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
            eoffsetSlider->setValue(aerolab->intEoffset());
        }

        double cda = ride->ride()->getTag("aerolab.Cda", "0").toDouble();
        if (cda>0) {
            int value = 10000 * cda;
            aerolab->setIntCda(value);
            cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
            cdaSlider->setValue(aerolab->intCda());
        }

        double crr = ride->ride()->getTag("customCrr",ride->ride()->getTag("notio.crr", "0")).toDouble();
        if (crr>0) {
            int value = 1000000 * crr;
            aerolab->setIntCrr(value);
            crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
            crrSlider->setValue(aerolab->intCrr());
        }

        QString comment = ride->ride()->getTag("aerolab.Comment", "");
        if (comment.length()>0) {
            commentEdit->setText(comment);
        }
    }

    refresh(ride, true);

    allZoomer->setZoomBase();
}

void
NotioVEWindow::setCrrFromText(const QString text) {
    int value = 1000000 * text.toDouble();
    if (aerolab->intCrr() != value) {
        aerolab->setIntCrr(value);
        //crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
        crrSlider->setValue(aerolab->intCrr());
        RideItem *ride = context->rideItem();
        refresh(ride, false);
    }
}

void
NotioVEWindow::setCrrFromSlider() {

    if (aerolab->intCrr() != crrSlider->value()) {
        aerolab->setIntCrr(crrSlider->value());
        //crrQLCDNumber->display(QString("%1").arg(aerolab->getCrr()));
        crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setCdaFromText(const QString text) {
    int value = 10000 * text.toDouble();
    if (aerolab->intCda() != value) {
        aerolab->setIntCda(value);
        //cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
        cdaSlider->setValue(aerolab->intCda());
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setCdaFromSlider() {

    if (aerolab->intCda() != cdaSlider->value()) {
        aerolab->setIntCda(cdaSlider->value());
        //cdaQLCDNumber->display(QString("%1").arg(aerolab->getCda()));
        cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setTotalMassFromText(const QString text) {
    int value = 100 * text.toDouble();
    if (value == 0)
        value = 1; // mass can not be zero !
    if (aerolab->intTotalMass() != value) {
        aerolab->setIntTotalMass(value);
        //mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()));
        mSlider->setValue(aerolab->intTotalMass());
        RideItem *ride = context->rideItem();
        refresh(ride, false);
    }
}

void
NotioVEWindow::setTotalMassFromSlider() {

    if (aerolab->intTotalMass() != mSlider->value()) {
        aerolab->setIntTotalMass(mSlider->value());
        //mQLCDNumber->display(QString("%1").arg(aerolab->getTotalMass()));
        mLineEdit->setText(QString("%1").arg(aerolab->getTotalMass()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setFactorFromText(const QString text) {
    int value = 10000 * text.toDouble();
    if (aerolab->intRiderFactor() != value) {
        cout << "in SetFactorFromText " << value << endl;
        cout << "call setIntRiderFactor " << value << endl;
        aerolab->setIntRiderFactor(value);
        factorSlider->setValue(aerolab->intRiderFactor());
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setFactorFromSlider() {

    if (aerolab->intRiderFactor() != factorSlider->value()) {
        cout << "in setFactorFromSlider " << endl;
        cout << "call setIntRiderFactor " << endl;
        aerolab->setIntRiderFactor(factorSlider->value());
        //rhoQLCDNumber->display(QString("%1").arg(aerolab->getRiderFactor()));
        factorLineEdit->setText(QString("%1").arg(aerolab->getRiderFactor()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}
void
NotioVEWindow::setExponentFromText(const QString text) {
    int value = 10000 * text.toDouble();
    if (aerolab->intRiderExponent() != value) {
        cout << "in setExponentFromText " << value << endl;
        cout << "call setIntRiderExponent " << value << endl;
        aerolab->setIntRiderExponent(value);
        exponentSlider->setValue(aerolab->intRiderExponent());
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setExponentFromSlider() {

    if (aerolab->intRiderExponent() != exponentSlider->value()) {
        cout << "in setExponentFromSlider" << endl;
        cout << "call setIntRiderExponent" << endl;
        aerolab->setIntRiderExponent(exponentSlider->value());
        exponentLineEdit->setText(QString("%1").arg(aerolab->getRiderExponent()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setEtaFromText(const QString text) {
    int value = 10000 * text.toDouble();
    if (aerolab->intEta() != value) {
        aerolab->setIntEta(value);
        //etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
        etaSlider->setValue(aerolab->intEta());
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setComment(const QString ) {
    if (hasNewParametersInRide()) {
        btnSave->setEnabled(true);
    } else
        btnSave->setEnabled(false);
}

void
NotioVEWindow::setEtaFromSlider() {

    if (aerolab->intEta() != etaSlider->value()) {
        aerolab->setIntEta(etaSlider->value());
        //etaQLCDNumber->display(QString("%1").arg(aerolab->getEta()));
        etaLineEdit->setText(QString("%1").arg(aerolab->getEta()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setEoffsetFromText(const QString text) {
    int value = 100 * text.toDouble();
    if (aerolab->intEoffset() != value) {
        aerolab->setIntEoffset(value);
        //eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
        eoffsetSlider->setValue(aerolab->intEoffset());
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setEoffsetFromSlider() {

    if (aerolab->intEoffset() != eoffsetSlider->value()) {
        aerolab->setIntEoffset(eoffsetSlider->value());
        //eoffsetQLCDNumber->display(QString("%1").arg(aerolab->getEoffset()));
        eoffsetLineEdit->setText(QString("%1").arg(aerolab->getEoffset()) );
        RideItem *ride = myRideItem;
        refresh(ride, false);
    }
}

void
NotioVEWindow::setAutoEoffset(int value)
{
    aerolab->setAutoEoffset(value);
}

void
NotioVEWindow::setConstantAlt(int value)
{
    aerolab->setConstantAlt(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride, false);
}

void
NotioVEWindow::setGarminON(int value)
{
    aerolab->setGarminON(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride, false);
}

void
NotioVEWindow::setWindOn(int value)
{
    aerolab->setWindOn(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride, false);
}

void
NotioVEWindow::setByDistance(int value)
{
    aerolab->setByDistance(value);
    // refresh
    RideItem *ride = myRideItem;
    refresh(ride, false);
}

void
NotioVEWindow::doEstCdACrr()
{
    RideItem *ride = context->rideItem();
    /* Estimate Crr&Cda */
    const QString errMsg = aerolab->estimateCdACrr(ride);
    if (errMsg.isEmpty()) {
        /* Update Crr/Cda values values in UI */
        crrLineEdit->setText(QString("%1").arg(aerolab->getCrr()) );
        crrSlider->setValue(aerolab->intCrr());
        cdaLineEdit->setText(QString("%1").arg(aerolab->getCda()) );
        cdaSlider->setValue(aerolab->intCda());
        /* Refresh */
        refresh(ride, false);
    } else {
        /* report error: insufficient data to estimate Cda&Crr */
        QMessageBox::warning(this, tr("Estimate CdA and Crr"), errMsg);
    }
}


void
NotioVEWindow::zoomInterval(IntervalItem *which) {
    QwtDoubleRect rect;

    if (!aerolab->byDistance()) {
        rect.setLeft(which->start/60);
        rect.setRight(which->stop/60);
    } else {
        rect.setLeft(which->startKM);
        rect.setRight(which->stopKM);
    }
    rect.setTop(aerolab->veCurve->maxYValue()*1.1);
    rect.setBottom(aerolab->veCurve->minYValue()-10);
    allZoomer->zoom(rect);

    aerolab->recalc(false);
}

void NotioVEWindow::intervalSelected()
{
    RideItem *ride = myRideItem;
    if ( !ride || m_isBlank)
    {
        return;
    }

    // set the elevation data
    refresh(ride, true);
}

double NotioVEWindow::getCanvasTop() const
{
    const QwtDoubleRect &canvasRect = allZoomer->zoomRect();
    return canvasRect.top();
}

double NotioVEWindow::getCanvasBottom() const
{
    const QwtDoubleRect &canvasRect = allZoomer->zoomRect();
    return canvasRect.bottom();
}

void NotioVEWindow::saveParametersInRide()
{
    if (hasNewParametersInRide()) {
        rideItem()->ride()->setTag("aerolab.Cda", QString("%1").arg(aerolab->getCda()));
        rideItem()->ride()->setTag("customCrr", QString("%1").arg(aerolab->getCrr()));
        rideItem()->ride()->setTag("customWeight", QString("%1").arg(aerolab->getTotalMass()));
        rideItem()->ride()->setTag("customMecEff", QString("%1").arg(aerolab->getEta()));
        rideItem()->ride()->setTag("customRiderFactor", QString("%1").arg(aerolab->getRiderFactor()));
        rideItem()->ride()->setTag("customExponent", QString("%1").arg(aerolab->getRiderExponent()));
        rideItem()->ride()->setTag("aerolab.EOffset", QString("%1").arg(aerolab->getEoffset()));
        rideItem()->ride()->setTag("aerolab.Comment", QString("%1").arg(commentEdit->text()));

        // For compatibility issues, rides computed with this version will be compatible with previous GC version.
        // To be removed in future release.
        rideItem()->ride()->setTag("notio.crr", QString("%1").arg(aerolab->getCrr()));
        rideItem()->ride()->setTag("notio.riderWeight", QString("%1").arg(aerolab->getTotalMass()));
        rideItem()->ride()->setTag("notio.efficiency", QString("%1").arg(aerolab->getEta()));
        rideItem()->ride()->setTag("notio.riderFactor", QString("%1").arg(aerolab->getRiderFactor()));
        rideItem()->ride()->setTag("notio.riderExponent", QString("%1").arg(aerolab->getRiderExponent()));
        // ==================================

        rideItem()->setDirty(true);

        context->mainWindow->saveSilent(context, rideItem());
        btnSave->setEnabled(false);
    }
}

bool NotioVEWindow::hasNewParametersInRide()
{
    bool newValues = false;

    if (rideItem()->ride()->getTag("customWeight", "0").toDouble() != aerolab->getTotalMass()) {
        newValues = true;
        //qDebug() << "new Total Weight" << rideItem()->ride()->getTag("notio.riderWeight", "0").toDouble() << aerolab->getTotalMass();
    }
    if (rideItem()->ride()->getTag("customMecEff", "0").toDouble() != aerolab->getEta()) {
        newValues = true;
        //qDebug() << "new Eta" << rideItem()->ride()->getTag("notio.efficiency", "0").toDouble() << aerolab->getEta();
    }
    if (rideItem()->ride()->getTag("customRiderFactor", "0").toDouble() != aerolab->getRiderFactor()) {
        newValues = true;
        qDebug() << "new RiderFactor" << rideItem()->ride()->getTag("customRiderFactor", "0").toDouble() << aerolab->getRiderFactor();
    }
    if (rideItem()->ride()->getTag("customExponent", "0").toDouble() != aerolab->getRiderExponent()) {
        newValues = true;
        qDebug() << "new Rider Exponent" << rideItem()->ride()->getTag("customExponent", "0").toDouble() << aerolab->getRiderExponent();
    }
    if (rideItem()->ride()->getTag("aerolab.EOffset", "0").toDouble() != aerolab->getEoffset()) {
        newValues = true;
        //qDebug() << "new EOffset" << rideItem()->ride()->getTag("aerolab.EOffset", "0").toDouble() << aerolab->getEoffset();
    }
    if (rideItem()->ride()->getTag("aerolab.Cda", "0").toDouble() != aerolab->getCda()) {
        newValues = true;
        //qDebug() << "new Cda" << rideItem()->ride()->getTag("aerolab.Cda", "0").toDouble() << aerolab->getCda();
    }
    if (rideItem()->ride()->getTag("customCrr", "0").toDouble() != aerolab->getCrr()) {
        newValues = true;
        //qDebug() << "new Crr" << rideItem()->ride()->getTag("notio.crr", "0").toDouble() << aerolab->getCrr();
    }
    if (rideItem()->ride()->getTag("aerolab.Comment", "") != commentEdit->text()) {
        newValues = true;
        //qDebug() << "new Comment" << rideItem()->ride()->getTag("aerolab.Comment", "") << commentEdit->text();
    }

    return newValues;
}

void NotioVEWindow::refresh(RideItem *_rideItem, bool newzoom) {
    if (hasNewParametersInRide()) {
        btnSave->setEnabled(true);
    } else
        btnSave->setEnabled(false);

    cout << "got a refresh " << newzoom << endl;

    if ( newzoom )
        newZoom = true;

    if ( newZoom )
        aerolab->setData(_rideItem, newzoom);

    return;
}
