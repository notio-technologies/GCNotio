/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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


#include "Tab.h"
#include "Views.h"
#include "Athlete.h"
#include "RideCache.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "MainWindow.h"
#include "Colors.h"

#include <QPaintEvent>

Tab::Tab(Context *context) : QWidget(context->mainWindow), context(context)
{
    context->tab = this;

    setContentsMargins(0,0,0,0);
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    views = new QStackedWidget(this);
    views->setContentsMargins(0,0,0,0);
    main->addWidget(views);

    // all the stack views for the controls
    masterControls = new QStackedWidget(this);
    masterControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    masterControls->setCurrentIndex(0);
    masterControls->setContentsMargins(0,0,0,0);

    // Home
    homeControls = new QStackedWidget(this);
    homeControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    homeControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(homeControls);
    homeView = new HomeView(context, homeControls);
    views->addWidget(homeView);

    // Analysis
    analysisControls = new QStackedWidget(this);
    analysisControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    analysisControls->setCurrentIndex(0);
    analysisControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(analysisControls);
    analysisView = new AnalysisView(context, analysisControls);
    views->addWidget(analysisView);

    // Diary
    diaryControls = new QStackedWidget(this);
    diaryControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    diaryControls->setCurrentIndex(0);
    diaryControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(diaryControls);
    diaryView = new DiaryView(context, diaryControls);
    views->addWidget(diaryView);

    // Train
    trainControls = new QStackedWidget(this);
    trainControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    trainControls->setCurrentIndex(0);
    trainControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(trainControls);
    trainView = new TrainView(context, trainControls);
    views->addWidget(trainView);

    // the dialog box for the chart settings
    chartSettings = new ChartSettings(this, masterControls);
    chartSettings->setMaximumWidth(650);
    chartSettings->setMaximumHeight(600);
    chartSettings->hide();

    // cpx aggregate cache check
    connect(context,SIGNAL(rideSelected(RideItem*)), this, SLOT(rideSelected(RideItem*)));

    // selects the latest ride in the list:
    // first skipping those in the future
    QDateTime now = QDateTime::currentDateTime();
    for (int i=context->athlete->rideCache->rides().count(); i>0; --i) {
        if (context->athlete->rideCache->rides()[i-1]->dateTime <= now) {
            context->athlete->selectRideFile(context->athlete->rideCache->rides()[i-1]->fileName);
            break;
        }
    }
    // otherwise just the latest
    if (context->currentRideItem() == NULL && context->athlete->rideCache->rides().count() != 0) 
        context->athlete->selectRideFile(context->athlete->rideCache->rides().last()->fileName);
}

Tab::~Tab()
{
    delete analysisView;
    delete homeView;
    delete trainView;
    delete diaryView;
    delete views;
}

RideNavigator *
Tab::rideNavigator()
{
    return analysisView->rideNavigator();
}

void
Tab::close()
{
    analysisView->saveState();
    homeView->saveState();
    trainView->saveState();
    diaryView->saveState();

    analysisView->close();
    homeView->close();
    trainView->close();
    diaryView->close();
}

/******************************************************************************
 * MainWindow integration with Tab / TabView (mostly pass through)
 *****************************************************************************/

bool Tab::hasBottom() { return view(currentView())->hasBottom(); }
bool Tab::isBottomRequested() { return view(currentView())->isBottomRequested(); }
void Tab::setBottomRequested(bool x) { view(currentView())->setBottomRequested(x); }
void Tab::setSidebarEnabled(bool x) { view(currentView())->setSidebarEnabled(x); }
bool Tab::isSidebarEnabled() { return view(currentView())->sidebarEnabled(); }
void Tab::toggleSidebar() { view(currentView())->setSidebarEnabled(!view(currentView())->sidebarEnabled()); }
void Tab::setTiled(bool x) { view(currentView())->setTiled(x); }
bool Tab::isTiled() { return view(currentView())->isTiled(); }
void Tab::toggleTile() { view(currentView())->setTiled(!view(currentView())->isTiled()); }
void Tab::resetLayout() { view(currentView())->resetLayout(); }
void Tab::addChart(GcWinID i) { view(currentView())->addChart(i); }
void Tab::addIntervals() { analysisView->addIntervals(); }

void Tab::setRide(RideItem*ride) 
{ 
    analysisView->setRide(ride);
    homeView->setRide(ride);
    trainView->setRide(ride);
    diaryView->setRide(ride);
}

TabView *
Tab::view(int index)
{
    switch(index) {
        case 0 : return homeView;
        default:
        case 1 : return analysisView;
        case 2 : return diaryView;
        case 3 : return trainView;
    }
}

void
Tab::selectView(int index)
{
    // first we deselect the current
    view(views->currentIndex())->setSelected(false);

    // now select the real one
    views->setCurrentIndex(index);
    view(index)->setSelected(true);
    masterControls->setCurrentIndex(index);
    context->setIndex(index);
}

void
Tab::rideSelected(RideItem*)
{
    // update the ride property on all widgets
    // to let them know they need to replot new
    // selected ride (now the tree is up to date)
    setRide(context->ride);

    // notify that the intervals have been cleared too
    context->notifyIntervalsChanged();
}

ProgressLine::ProgressLine(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    setFixedHeight(6 *dpiYFactor);
    hide();

    connect(context, SIGNAL(refreshStart()), this, SLOT(show()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(hide()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(show())); // we might miss 1st one
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(repaint()));
}

void
ProgressLine::paintEvent(QPaintEvent *)
{

    // nothing for test...
    QColor translucentGray = GColor(CPLOTMARKER);
    translucentGray.setAlpha(240);
    QColor translucentWhite = GColor(CPLOTBACKGROUND);

    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, translucentWhite);

    // progressbar
    QRectF progress(0, 0, (double(context->athlete->rideCache->progress()) / 100.0f) * double(width()), height());
    painter.fillRect(progress, translucentGray);
    painter.restore();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CustomProgressLine::CustomProgressLine
///        Constructor.
///
/// \param[in] parent   Parent widget.
/// \param[in] context  Context.
///////////////////////////////////////////////////////////////////////////////
CustomProgressLine::CustomProgressLine(QWidget *parent, Context *context) : QWidget(parent), m_context(context)
{
    setFixedHeight(static_cast<int>(6 * dpiYFactor));
    hide();

    connect(context, SIGNAL(customProgressStart()), SLOT(start()));
    connect(context, SIGNAL(customProgressEnd()), SLOT(stop()));
    connect(context, SIGNAL(customProgressUpdate(int)), SLOT(refresh(int)), Qt::DirectConnection);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CustomProgressLine::paintEvent
///        This method is called when a paint event occurs. It paints and display
///        the progress bar.
///////////////////////////////////////////////////////////////////////////////
void CustomProgressLine::paintEvent(QPaintEvent *)
{
    // Set colors.
    QColor wTranslucentGray = GColor(CPOWER);
    wTranslucentGray.setAlpha(240);
    QColor wTranslucentWhite = GColor(CPLOTBACKGROUND);

    // Setup a painter and the area to paint.
    QPainter wPainter(this);

    wPainter.save();
    QRect wAll(0, 0, width(), height());

    // Fill.
    wPainter.setPen(Qt::NoPen);
    wPainter.fillRect(wAll, wTranslucentWhite);

    // Progress bar.
    QRectF wProgress(0, 0, (m_progress / 100.0) * double(width()), height());
    wPainter.fillRect(wProgress, wTranslucentGray);
    wPainter.restore();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CustomProgressLine::refresh
///        This method updates the progress bar value and repaint it.
///
/// \param[in] iValue   Updated value.
///////////////////////////////////////////////////////////////////////////////
void CustomProgressLine::refresh(int iValue)
{
    m_progress = iValue;
    show();
    repaint();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CustomProgressLine::start
///        This method shows the progress bar to the user and set the wait
///        cursor.
///////////////////////////////////////////////////////////////////////////////
void CustomProgressLine::start()
{
    m_progress = 0;

    // Show waiting cursor.
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    show();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CustomProgressLine::stop
///        This method hides the progress bar and restore the cursor.
///////////////////////////////////////////////////////////////////////////////
void CustomProgressLine::stop()
{
    // Show waiting cursor.
    QApplication::restoreOverrideCursor();
    hide();
}
