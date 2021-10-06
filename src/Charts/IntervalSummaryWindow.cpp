/*
 * Copyright (c) 2011 Eric Brandt (eric.l.brandt@gmail.com)
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
#include "Athlete.h"
#include "RideFile.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "IntervalSummaryWindow.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Colors.h"
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

// Initialize fake rides.
RideFile *IntervalSummaryWindow::m_fake = nullptr;
RideFile *IntervalSummaryWindow::m_notFake = nullptr;

IntervalSummaryWindow::IntervalSummaryWindow(QWidget *iParent, Context *context) : context(context), m_parent(iParent)
{
    setWindowTitle(tr("Interval Summary"));
    setReadOnly(true);
    //XXXsetEnabled(false); // stop the fucking thing grabbing keyboard focus FFS.
    setFrameStyle(QFrame::NoFrame);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    verticalScrollBar()->setStyle(cde);
#endif

#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
    connect(context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(intervalSelected()));

    setHtml(GCColor::css() + "<body></body>");
}

IntervalSummaryWindow::~IntervalSummaryWindow() {
}

void IntervalSummaryWindow::intervalSelected()
{
    // if no ride available don't bother - just reset for color changes
    RideItem *rideItem = const_cast<RideItem*>(context->currentRideItem());

    if (rideItem == NULL || rideItem->intervalsSelected().count() == 0 || rideItem->ride() == NULL) {
        // no ride just update the colors
	    QString html = GCColor::css();
        html += "<body></body>";
	    setHtml(html);
	    return;
    }

    // summary is html
	QString html = GCColor::css();
    html += "<body>";

    // summarise all the intervals selected - this is painful!
    // now also summarises for entire ride EXCLUDING the intervals selected
    QString notincluding;
    if (rideItem->intervalsSelected().count()>1) html += summary(rideItem->intervalsSelected(), notincluding);

    // summary for each of the currently selected intervals
    foreach(IntervalItem *interval, rideItem->intervalsSelected()) html += summary(interval);

    // now add the excluding text
    html += notincluding;

    if (html == GCColor::css()+"<body>") html += "<i>" + tr("select an interval for summary info") + "</i>";

    html += "</body>";
	setHtml(html);
    return;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief IntervalSummaryWindow::resetFakeRides
///        This methods creates fake rides for the intervals summary text.
///////////////////////////////////////////////////////////////////////////////
void IntervalSummaryWindow::resetFakeRides()
{
    // Delete fake rides.
    if (m_fake)
    {
        delete m_fake;
        m_fake = nullptr;
    }

    if (m_notFake)
    {
        delete m_notFake;
        m_notFake = nullptr;
    }

    RideFile *wRide = nullptr;
    if (context && context->ride)
        wRide = context->ride->ride();

    // Create fake rides.
    if (wRide)
    {
        // Included intervals.
        m_fake = new RideFile(wRide);

        // Excluded intervals.
        m_notFake = new RideFile(wRide);
    }
}

void
IntervalSummaryWindow::intervalHover(IntervalItem* x)
{
    // if we're not visible don't bother
    if (!isVisible()) return;

    // we already have summaries!
    if (x && x->rideItem()->intervalsSelected().count()) return;

    // its to clear, but if the current ride has selected intervals then we will ignore it
    RideItem *rideItem = const_cast<RideItem*>(context->currentRideItem());
    if (!x && rideItem && rideItem->intervalsSelected().count()) return;

    QString html = GCColor::css();
    html += "<body>";

    if (x == NULL) {
    	html += "<i>" + tr("select an interval for summary info") + "</i>";
    } else {
        html += summary(x);
    }
    html += "</body>";
    setHtml(html);
    return;
}

static bool contains(const RideFile*ride, QList<IntervalItem*> intervals, int index)
{
    foreach(IntervalItem *item, intervals) {
        int start = ride->timeIndex(item->start);
        int end = ride->timeIndex(item->stop);

        if (index >= start && index <= end) return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief contains
///        This function verify that a XDataSeries point is within the
///        intervals boundaries.
///
/// \param[in] iXDataSeries XDataSeries from which the point originated.
/// \param[in] iIntervals   Intervals' list.
/// \param[in] iIndex       XDataPoint index.
///
/// \return A bool value indicating if the point is contained in the intervals.
///////////////////////////////////////////////////////////////////////////////
static bool contains(const XDataSeries *iXDataSeries, QList<IntervalItem *> iIntervals, int iIndex)
{
    foreach(IntervalItem *item, iIntervals) {
        int start = iXDataSeries->timeIndex(item->start);
        int end = iXDataSeries->timeIndex(item->stop);

        if (iIndex >= start && iIndex <= end) return true;
    }
    return false;
}

QString IntervalSummaryWindow::summary(QList<IntervalItem*> intervals, QString &notincluding)
{
    bool wBuildFakeRides = (m_fake == nullptr) || (m_notFake == nullptr) || (m_parent && m_parent->property("ClassName").toString() == "AnalysisSidebar");

    // need a current rideitem
    if (!context->currentRideItem()) return "";

    // We need to create a special ridefile just for the selected intervals
    // to calculate the aggregated metrics because intervals can OVERLAP!
    // so we can't just aggregate the pre-computed metrics as this will lead
    // to overstated totals and skewed averages.
    const RideFile* ride = context->ride ? context->ride->ride() : NULL;

    if (wBuildFakeRides)
    {
        // Interval selection changed, need to rebuild fake rides.
        resetFakeRides();

        // for concatenating intervals
        RideFilePoint *last = NULL;
        double timeOff=0;
        double distOff=0;

        RideFilePoint *notlast = NULL;
        double nottimeOff=0;
        double notdistOff=0;

        for (int i = 0; i < ride->dataPoints().count(); ++i) {

            // append points for selected intervals
            const RideFilePoint *p = ride->dataPoints()[i];
            if (contains(ride, intervals, i)) {

                // drag back time/distance for data not included below
                if (notlast) {
                    nottimeOff = p->secs - notlast->secs;
                    notdistOff = p->km - notlast->km;
                } else {
                    nottimeOff = p->secs;
                    notdistOff = p->km;
                }

                m_fake->appendPoint(p->secs-timeOff, p->cad, p->hr, p->km-distOff, p->kph, p->nm,
                              p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance,
                              p->lte, p->rte, p->lps, p->rps,
                              p->lpco, p->rpco,
                              p->lppb, p->rppb, p->lppe, p->rppe,
                              p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                              p->smo2, p->thb,
                              p->rvert, p->rcad, p->rcontact, p->tcore, 0);

                // derived data
                last = m_fake->dataPoints().last();
                last->np = p->np;
                last->xp = p->xp;
                last->apower = p->apower;

            } else {

                // drag back time/distance for data not included above
                if (last) {
                    timeOff = p->secs - last->secs;
                    distOff = p->km - last->km;
                } else {
                    timeOff = p->secs;
                    distOff = p->km;
                }

                m_notFake->appendPoint(p->secs-nottimeOff, p->cad, p->hr, p->km-notdistOff, p->kph, p->nm,
                                 p->watts, p->alt, p->lon, p->lat, p->headwind, p->slope, p->temp, p->lrbalance,
                                 p->lte, p->rte, p->lps, p->rps,
                                 p->lpco, p->rpco,
                                 p->lppb, p->rppb, p->lppe, p->rppe,
                                 p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                                 p->smo2, p->thb,
                                 p->rvert, p->rcad, p->rcontact, p->tcore, 0);

                // derived data
                notlast = m_notFake->dataPoints().last();
                notlast->np = p->np;
                notlast->xp = p->xp;
                notlast->apower = p->apower;
            }
        }

        // For concatenating intervals XData.
        for (auto *xdataItr : context->ride->ride()->xdata())
        {
            // Selected intervals.
            XDataPoint *xLast = nullptr;
            timeOff=0;
            distOff=0;

            // Excluded intervals.
            XDataPoint *xNotlast = nullptr;
            nottimeOff=0;
            notdistOff=0;

            if (xdataItr == nullptr)
                continue;

            // Fake selected intervals XDataSeries.
            XDataSeries* xd = new XDataSeries();

            // Fake not included intervals XDataSeries.
            XDataSeries* notXd = new XDataSeries();

            xd->name = notXd->name = xdataItr->name;
            xd->valuename = notXd->valuename = xdataItr->valuename;
            xd->unitname = notXd->unitname = xdataItr->unitname;
            xd->valuetype = notXd->valuetype = xdataItr->valuetype;

            // Add XData series for selected intervals fake ride file.
            m_fake->addXData(xd->name, xd);

            // Add XData series for excluded intervals fake ride file.
            m_notFake->addXData(notXd->name, notXd);

            // Populate XData series data points.
            for (int i = 0; i < xdataItr->datapoints.count(); i++)
            {
                XDataPoint *point = xdataItr->datapoints[i];

                // Is in selected intervals.
                if (contains(xdataItr, intervals, i))
                {
                    // drag back time/distance for data not included below
                    if (xNotlast) {
                        nottimeOff = point->secs - xNotlast->secs;
                        notdistOff = point->km - xNotlast->km;
                    } else {
                        nottimeOff = point->secs;
                        notdistOff = point->km;
                    }

                    XDataPoint *pt = new XDataPoint(*point);
                    pt->secs = point->secs - timeOff;
                    pt->km = point->km - distOff;
                    xd->datapoints.append(pt);

                    xLast = xd->datapoints.last();
                }
                // Not in selected intervals.
                else
                {
                    // drag back time/distance for data not included above
                    if (xLast) {
                        timeOff = point->secs - xLast->secs;
                        distOff = point->km - xLast->km;
                    } else {
                        timeOff = point->secs;
                        distOff = point->km;
                    }

                    XDataPoint *pt = new XDataPoint(*point);
                    pt->secs = point->secs - nottimeOff;
                    pt->km = point->km - notdistOff;
                    notXd->datapoints.append(pt);

                    xNotlast = notXd->datapoints.last();
                }
            }
        }
    }

    QString s;
    if (appsettings->contains(GC_SETTINGS_INTERVAL_METRICS))
        s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS).toString();
    else
        s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
    QStringList intervalMetrics = s.split(",");
    const RideMetricFactory &factory = RideMetricFactory::instance();

    // build fake rideitem and compute metrics
    RideItem *fake;
    fake = new RideItem(m_fake, context);
    fake->setFrom(*const_cast<RideItem*>(context->currentRideItem()), true); // this wipes ride_ so put back
    fake->ride_ = m_fake;
    fake->getWeight();
    fake->intervals_.clear(); // don't accidentally wipe these!!!!
    fake->samples = m_fake->dataPoints().count() > 0;
    QHash<QString,RideMetricPtr> metrics = RideMetric::computeMetrics(fake, Specification(), intervalMetrics);

    // build fake for not in intervals
    RideItem *notfake;
    notfake = new RideItem(m_notFake, context);
    notfake->setFrom(*const_cast<RideItem*>(context->currentRideItem()), true); // this wipes ride_ so put back
    notfake->ride_ = m_notFake;
    notfake->getWeight();
    notfake->intervals_.clear(); // don't accidentally wipe these!!!!
    notfake->samples = m_notFake->dataPoints().count() > 0;
    QHash<QString,RideMetricPtr> notmetrics = RideMetric::computeMetrics(notfake, Specification(), intervalMetrics);

    // create temp interval item to use by interval summary
    IntervalItem temp(NULL, "", 0, 0, 0, 0, 0, Qt::black, false, RideFileInterval::USER);

    // pack the metrics away and clean up if needed
    temp.metrics().fill(0, factory.metricCount());

    // NOTE INCLUDED
    // snaffle away all the computed values into the array
    QHashIterator<QString, RideMetricPtr> i(metrics);
    while (i.hasNext()) {
        i.next();
        temp.metrics()[i.value()->index()] = i.value()->value();
    }

    // clean any bad values
    for(int j=0; j<factory.metricCount(); j++)
        if (std::isinf(temp.metrics()[j]) || std::isnan(temp.metrics()[j]))
            temp.metrics()[j] = 0.00f;

    // set name
    temp.name = QString(tr("%1 selected intervals")).arg(intervals.count());
    temp.rideItem_ = fake;

    QString returning = summary(&temp);

    if (m_notFake->dataPoints().count()) {
        // EXCLUDING
        // snaffle away all the computed values into the array
        // pack the metrics away and clean up if needed
        temp.metrics().fill(0, factory.metricCount());

        QHashIterator<QString, RideMetricPtr> i(notmetrics);
        while (i.hasNext()) {
            i.next();
            temp.metrics()[i.value()->index()] = i.value()->value();
        }

        // clean any bad values
        for(int j=0; j<factory.metricCount(); j++)
            if (std::isinf(temp.metrics()[j]) || std::isnan(temp.metrics()[j]))
                temp.metrics()[j] = 0.00f;

        // set name
        temp.name = QString(tr("Excluding %1 selected")).arg(intervals.count());
        temp.rideItem_ = notfake;

        // use standard method from above
        notincluding = summary(&temp);
    }

    // remove references temporary / fakes get wiped
    temp.rideItem_ = NULL;

    // zap references to real, and delete temporary ride item
    fake->ride_ = NULL;
    delete fake;
    
    notfake->ride_ = NULL;
    delete notfake;

    return returning;
}

QString IntervalSummaryWindow::summary(IntervalItem *interval)
{
    QString html;

    bool useMetricUnits = context->athlete->useMetricUnits;

    QString s;
    if (appsettings->contains(GC_SETTINGS_INTERVAL_METRICS))
        s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS).toString();
    else
        s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
    QStringList intervalMetrics = s.split(",");

    html += "<b>" + interval->name + "</b>";
    html += "<table align=\"center\" width=\"90%\" ";
    html += "cellspacing=0 border=0>";

    RideMetricFactory &factory = RideMetricFactory::instance();
    foreach (QString symbol, intervalMetrics) {
        const RideMetric *m = factory.rideMetric(symbol);
        if (!m) continue;

        // skip metrics that are not relevant for this ride
        if (!interval->rideItem() || m->isRelevantForRide(interval->rideItem()) == false) continue;

        html += "<tr>";
        // left column (names)
        html += "<td align=\"right\" valign=\"bottom\">" + m->name() + "</td>";

        // right column (values)
        QString s("<td align=\"center\">%1</td>");
        html += s.arg(interval->getStringForSymbol(symbol, useMetricUnits));
        html += "<td align=\"left\" valign=\"bottom\">";
        if (m->units(useMetricUnits) == "seconds" ||
            m->units(useMetricUnits) == tr("seconds"))
            ; // don't do anything
        else if (m->units(useMetricUnits).size() > 0)
            html += m->units(useMetricUnits);
        html += "</td>";

        html += "</tr>";

    }
    html += "</table>";

    return html;
}
