/*
 * Copyright (c) 2017 Ahmed Id-Oumohmed
 *               2018 Michaël Beaulieu (michael.beaulieu@notiotechnologies.com)
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

#include "RideMap3DWindow.h"
#include "ColorZonesBar.h"
#include "RideMap3DPlot.h"

#include "Units.h"
#include "UserData.h"
#include "HelpWhatsThis.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Specification.h"
#include "IntervalTreeView.h"
#include "RideFile.h"
#include "TabView.h"
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include "MainWindow.h"
#include <math.h>
#include <QDebug>

#ifdef NOWEBKIT
#include <QtWebChannel>
#endif

const QString RideMap3DWindow::kMap3DUrl = "https://cloud.notiokonect.com/ridevisgc/v1";
const bool RideMap3DWindow::isUsingExtraData = false;

///////////////////////////////////////////////////////////////////////////////
/// \brief operator +=
///        This method overloads the += operator to make it easier to build the
///        3D wall data string composed of points to be send through javascript.
///
/// \param[in/out]  ioResult    Temporary output string.
/// \param[in]      iPoint      3D map point to concatenate.
///
/// \return The output string result.
///////////////////////////////////////////////////////////////////////////////
QString &operator+=(QString &ioResult, const RideMap3DWindow::s_map3DPoint &iPoint)
{
    // A point is defines this way in the javascript string.
    ioResult += QString("{point:[%1,%2,%3],dist:%4,value:%5,sec:%6}")
            .arg(QString::number(iPoint.mLon), QString::number(iPoint.mLat),
                 QString::number(iPoint.mAlt), QString::number(iPoint.mDist),
                 QString::number(iPoint.mValue), QString::number(iPoint.mSecs));
    return ioResult;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::RideMap3DWindow
///        Constructor.
///
/// \param[in] iContext Context pointer.
///////////////////////////////////////////////////////////////////////////////
RideMap3DWindow::RideMap3DWindow(Context *iContext)
    : GcChartWindow(iContext), m_context(iContext)
{
    setProperty("ClassName", "RideMap3DWindow");
    // Chart main layout.
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(2, 0, 2, 2);

    QHBoxLayout *wRideWallSelectionLayout = new QHBoxLayout;
    wRideWallSelectionLayout->setContentsMargins(5, 5, 5, 5);

    // Creates wall selection menu.
    QString wSelToolTipStr = tr("Choose metric to display on the map.");
    m_rideWallSelection = new QComboBox;
    m_rideWallSelection->setToolTip(wSelToolTipStr);

    m_rideWallSelectionSettings = new QComboBox;
    m_rideWallSelectionSettings->setToolTip(wSelToolTipStr);

    initializeMetricList();

    m_rideWallSelection->setCurrentIndex(0);
    m_rideWallSelectionSettings->setCurrentIndex(0);

    QPalette wPalette;
    wPalette.setColor(QPalette::Foreground,Qt::white);

    QLabel *wSelectText = new QLabel(tr("Metric Selection"));
    wSelectText->setPalette(wPalette);
    wRideWallSelectionLayout->addStretch();
    wRideWallSelectionLayout->addWidget(wSelectText, 0, Qt::AlignCenter);
    wRideWallSelectionLayout->addSpacing(5);
    wRideWallSelectionLayout->addWidget(m_rideWallSelection, 0, Qt::AlignCenter);
    wRideWallSelectionLayout->addStretch();

    m_mainLayout->addLayout(wRideWallSelectionLayout);

#ifdef NOWEBKIT
    m_view = new QWebEngineView(this);
#else
    m_view = new QWebView();
#endif
    m_view->setPage(new Map3DWebPage());
    m_view->setContentsMargins(0, 0, 0, 0);
    m_view->page()->view()->setContentsMargins(0, 0, 0, 0);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_view->setAcceptDrops(false);

    HelpWhatsThis *help = new HelpWhatsThis(m_view);
    m_view->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Map3D));

    m_mainLayout->addWidget(m_view);

    m_webBridge = new Map3DWebBridge(m_context, this);
#ifdef NOWEBKIT
    QWebChannel *wChannel = new QWebChannel(m_view->page());

    // Set the web channel to be used by the page
    // See http://doc.qt.io/qt-5/qwebenginepage.html#setWebChannel
    m_view->page()->setWebChannel(wChannel);

    m_cloudUrl = new QUrl(kMap3DUrl);

    // Register QObjects to be exposed to JavaScript.
    wChannel->registerObject(QStringLiteral("m_webBridge"), m_webBridge);
#endif

    // FullPlot below the map.
    m_smallPlot3D = new RideMap3DPlot(m_context, this);
    m_smallPlot3D->setMaximumHeight(200);
    m_smallPlot3D->setMinimumHeight(200);
    m_smallPlot3D->setVisible(false);
    m_mainLayout->addWidget(m_smallPlot3D);
    m_mainLayout->setStretch(1, 20);

    setChartLayout(m_mainLayout);

    QAction *wResetView = new QAction(tr("Reset View"), this);
    wResetView->setToolTip(tr("Reset the map to the current ride."));
    addAction(wResetView);

    connect(wResetView, SIGNAL(triggered()), this, SLOT(resetView()));

    QAction *wReloadMap = new QAction(tr("Reload Map"), this);
    wReloadMap->setToolTip(tr("Reload 3D map."));
    addAction(wReloadMap);

    connect(wReloadMap, SIGNAL(triggered()), SLOT(loadMap3DView()));

    // Chart setting window.
    QWidget *wSettingsWidget = new QWidget(this);
    QVBoxLayout *wSettingsCtrlLayout = new QVBoxLayout(wSettingsWidget);
    wSettingsWidget->setContentsMargins(0, 0, 0, 0);

    HelpWhatsThis *helpSettings = new HelpWhatsThis(wSettingsWidget);
    wSettingsWidget->setWhatsThis(helpSettings->getWhatsThisText(HelpWhatsThis::ChartRides_Map3D));

    // Create settings disposition into tab widget.
    QTabWidget *wSettingsTab = new QTabWidget(this);
    wSettingsCtrlLayout->addWidget(wSettingsTab);

    // GUI controls.
    QWidget *wBasicSettings = new QWidget(this);
    QVBoxLayout *wBasicCtrlLayout = new QVBoxLayout(wBasicSettings);

    QFormLayout *wGuiControls = new QFormLayout;
    wBasicCtrlLayout->addLayout(wGuiControls);
    wBasicCtrlLayout->addStretch();
    wSettingsTab->addTab(wBasicSettings, tr("Basic"));

    HelpWhatsThis *basicHelp = new HelpWhatsThis(wBasicSettings);
    wBasicSettings->setWhatsThis(basicHelp->getWhatsThisText(HelpWhatsThis::ChartRides_Map3D_Config_Basic));

    // Show Map 3D plot.
    m_fullPlotCheckBox = new QCheckBox();
    m_fullPlotCheckBox->setChecked(false);  // Unchecked by default.
    m_fullPlotCheckBox->setToolTip(tr("Displays the metric graph below the map."));

    // Show altitude on the Map 3D plot.
    m_showFullPlotAlt = new QCheckBox();
    m_showFullPlotAlt->setToolTip(tr("Display altitude on the graph below the map."));
    m_showFullPlotAltText = new QLabel("\t" + tr("Show altitude"));
    m_showFullPlotAlt->setChecked(true);    // Checked by default but disabled
    m_showFullPlotAlt->setEnabled(false);
    m_showFullPlotAltText->setEnabled(false);

    // Displayed sections duration.
    m_sectionDuration = new QSpinBox();
    m_sectionDuration->setToolTip(tr("Set the 3D wall rendering sections duration. (15 to 60 seconds)"));
    m_sectionDuration->setRange(15, 60);
    m_sectionDuration->setValue(30);
    m_sectionDuration->setSuffix(" " + tr("seconds"));

    // Show interval window.
    m_showIntvlOvl = new QCheckBox();
    m_showIntvlOvl->setChecked(true);
    m_showIntvlOvl->setToolTip(tr("Show interval summary overlay."));

    // Reset default settings.
    // Button to reset color bar to default values.
    m_resetDefaultButton = new QPushButton();

    m_resetDefaultButton->setIcon(QPixmap(":images/toolbar/refresh.png"));
    m_resetDefaultButton->setIconSize(QSize(20, 20));
    m_resetDefaultButton->setMaximumSize(QSize(25, 25));
    m_resetDefaultButton->setToolTip(tr("Reset metric to defaults."));
    m_resetDefaultButton->setFocusPolicy(Qt::NoFocus);
    m_resetDefaultButton->setDisabled(true);

    QHBoxLayout *wMetricSelLayout = new QHBoxLayout;
    wMetricSelLayout->addWidget(m_rideWallSelectionSettings);
    wMetricSelLayout->addWidget(m_resetDefaultButton);

    // Create the color zones bar.
    m_rideWallColorBar = new ColorZonesBar();
    m_rideWallColorBar->setToolTip(tr("Customize 3D wall colors."));

    wPalette = palette();

    // Set white background.
    wPalette.setColor(QPalette::Background, Qt::white);

    m_rideWallColorBar->setAutoFillBackground(true);
    m_rideWallColorBar->setPalette(wPalette);

    // Populate settings layout.
    wGuiControls->addRow(new QLabel(tr("Show Full Plot")), m_fullPlotCheckBox);
    wGuiControls->addRow(m_showFullPlotAltText, m_showFullPlotAlt);
    wGuiControls->addRow(new QLabel(tr("Show Intervals Overlay")), m_showIntvlOvl);
    wGuiControls->addRow(new QLabel(tr("Sections duration")), m_sectionDuration);
    wGuiControls->addRow(new QLabel(tr("Metric Selection")), wMetricSelLayout);
    wGuiControls->addRow(m_rideWallColorBar);

    connect(m_showIntvlOvl, SIGNAL(stateChanged(int)), SLOT(showIntvlOvlChanged(int)));
    connect(m_fullPlotCheckBox, SIGNAL(stateChanged(int)), SLOT(showFullPlotChanged(int)));
    connect(m_showFullPlotAlt, SIGNAL(stateChanged(int)), SLOT(showFullPlotAltChanged(int)));
    connect(m_sectionDuration, SIGNAL(valueChanged(int)), SLOT(currentRideRefreshWall()));
    connect(m_rideWallSelectionSettings, SIGNAL(currentIndexChanged(int)), SLOT(metricSettingsSelectionChanged(int)));
    connect(m_resetDefaultButton, SIGNAL(clicked()), SLOT(resetMetricDefault()));
    connect(m_rideWallColorBar, SIGNAL(colorBarUpdated(const QList<double>)), SLOT(changeWallPoints(const QList<double>)));
    connect(m_rideWallColorBar, SIGNAL(colorBarUpdated(const QList<QColor>)), SLOT(changeWallColors(const QList<QColor>)));

    // User data widget.
    m_customUserData = new QWidget(this);
    wSettingsTab->addTab(m_customUserData, tr("User Data"));
    m_customUserData->setContentsMargins(static_cast<int>(20 * dpiXFactor), static_cast<int>(20 * dpiYFactor),
                                         static_cast<int>(20 * dpiXFactor), static_cast<int>(20 * dpiYFactor));

    HelpWhatsThis *curvesHelp = new HelpWhatsThis(m_customUserData);
    m_customUserData->setWhatsThis(curvesHelp->getWhatsThisText(HelpWhatsThis::ChartRides_Map3D_Config_UserData));

    QVBoxLayout *customLayout = new QVBoxLayout(m_customUserData);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->setSpacing(static_cast<int>(5 * dpiXFactor));

    // Custom table.
    m_customTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    m_customTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    m_customTable->setColumnCount(2);
    m_customTable->horizontalHeader()->setStretchLastSection(true);
    QStringList headings;
    headings << tr("Name");
    headings << tr("Formula");
    m_customTable->setHorizontalHeaderLabels(headings);
    m_customTable->setSortingEnabled(false);
    m_customTable->verticalHeader()->hide();
    m_customTable->setShowGrid(false);
    m_customTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_customTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    customLayout->addWidget(m_customTable);
    connect(m_customTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(doubleClicked(int, int)));

    // Create custom buttons.
    m_editCustomButton = new QPushButton(tr("Edit"));
    connect(m_editCustomButton, SIGNAL(clicked()), this, SLOT(editUserData()));

    m_addCustomButton = new QPushButton("+");
    connect(m_addCustomButton, SIGNAL(clicked()), this, SLOT(addUserData()));

    m_deleteCustomButton = new QPushButton("-");
    connect(m_deleteCustomButton, SIGNAL(clicked()), this, SLOT(deleteUserData()));

#ifndef Q_OS_MAC
    m_upCustomButton = new QToolButton(this);
    m_downCustomButton = new QToolButton(this);
    m_upCustomButton->setArrowType(Qt::UpArrow);
    m_downCustomButton->setArrowType(Qt::DownArrow);
    m_upCustomButton->setFixedSize(static_cast<int>(20 * dpiXFactor), static_cast<int>(20 * dpiYFactor));
    m_downCustomButton->setFixedSize(static_cast<int>(20 * dpiXFactor), static_cast<int>(20 * dpiYFactor));
    m_addCustomButton->setFixedSize(static_cast<int>(20 * dpiXFactor), static_cast<int>(20 * dpiYFactor));
    m_deleteCustomButton->setFixedSize(static_cast<int>(20 * dpiXFactor), static_cast<int>(20 * dpiYFactor));
#else
    m_upCustomButton = new QPushButton(tr("Up"));
    m_downCustomButton = new QPushButton(tr("Down"));
#endif
    connect(m_upCustomButton, SIGNAL(clicked()), this, SLOT(moveUserDataUp()));
    connect(m_downCustomButton, SIGNAL(clicked()), this, SLOT(moveUserDataDown()));
    connect(this, SIGNAL(userDataChanged(qint32)), this, SLOT(configChanged(qint32)));

    // Set custom buttons layout.
    QHBoxLayout *wCustomButtons = new QHBoxLayout;
    wCustomButtons->setSpacing(static_cast<int>(2 * dpiXFactor));
    wCustomButtons->addWidget(m_upCustomButton);
    wCustomButtons->addWidget(m_downCustomButton);
    wCustomButtons->addStretch();
    wCustomButtons->addWidget(m_editCustomButton);
    wCustomButtons->addStretch();
    wCustomButtons->addWidget(m_addCustomButton);
    wCustomButtons->addWidget(m_deleteCustomButton);
    customLayout->addLayout(wCustomButtons);

    // Set chart controls.
    setControls(wSettingsWidget);
    setContentsMargins(0,0,0,0);

    // Put a helper on the screen for mouse over intervals...
    m_intervalOverlay = new IntervalSummaryWindow(this, m_context);
    addHelper(tr("Intervals"), m_intervalOverlay);

    //
    // connects
    //
#ifndef NOWEBKIT
    connect(m_view->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(updateFrame()));
#endif

    connect(m_view, SIGNAL(loadFinished(bool)), SLOT(loadMap3DFinished(bool)), Qt::DirectConnection);

    connect(m_rideWallSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(wallFactorySelectionChanged(int)));
    connect(this, SIGNAL(showControls()), this, SLOT(refreshColorBar()), Qt::QueuedConnection); // Controls need to appear first.
    connect(this, SIGNAL(rideItemChanged(RideItem*)), SLOT(rideSelected()));
    connect(m_context, SIGNAL(rideChanged(RideItem*)), SLOT(forceReplot()));
    connect(m_context, SIGNAL(intervalZoom(IntervalItem*)), SLOT(zoomInterval(IntervalItem *)));
    connect(m_context, SIGNAL(zoomOut()), SLOT(resetView()));
    connect(m_context, SIGNAL(configChanged(qint32)), SLOT(configChanged(qint32)));

    // First load 3D map view.
    loadMap3DView();

    configChanged(CONFIG_APPEARANCE);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::~RideMap3DWindow
///        Destructor.
///////////////////////////////////////////////////////////////////////////////
RideMap3DWindow::~RideMap3DWindow()
{
    qDebug() << Q_FUNC_INFO;

    // Delete web bridge if not null.
    if (m_webBridge)
    {
        delete m_webBridge;
        m_webBridge = nullptr;
    }

    // Delete URL object if not null.
    if (m_cloudUrl)
    {
        delete m_cloudUrl;
        m_cloudUrl = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::getUserData
///        This method gets the user data for the charts as string. It is used
///        to save it to the chart settings.
///
/// \return A string containing the user data settings.
///////////////////////////////////////////////////////////////////////////////
QString RideMap3DWindow::getUserData() const
{
    QString wReturning;
    foreach(UserData *wUserDataItr, m_userDataSeries)
        wReturning += wUserDataItr->settings();

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::getColorSettings
///        This method is called when saving the chart configuration.
///
/// \return The list of colors for the metrics.
///////////////////////////////////////////////////////////////////////////////
QString RideMap3DWindow::getColorSettings()
{
    QString wReturning;
    RideWallFactory &wFactory = RideWallFactory::instance();

    // Go through all metric walls.
    for (int i = 0; i < wFactory.count(); i++)
    {
        QString wColors;
        QString wName = wFactory[i]->getName();

        // Skip RideWall with no name of type DataSeries (none).
        if (wName.isEmpty() && (wFactory[i]->getType() == RideWallFactory::DATASERIES))
            continue;

        // Separate wall name from its colors list.
        wColors.append(wName + "@");

        // Create a string with the colors' name.
        bool wFirstPass = true;
        if (wFactory[i]->isDefaultColors() == false)
        {
            for (auto &wColorItr : wFactory[i]->colorsList())
            {
                // First color.
                if (wFirstPass)
                {
                    wFirstPass = false;
                    wColors.append(wColorItr.name());
                }
                // Next color.
                else
                {
                    wColors.append(QString("; %1").arg(wColorItr.name()));
                }
            }
        }

        // Walls separator character.
        wReturning.append(wColors + "%");
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::getRangeSettings
///        This method is called when saving the chart configuration.
///
/// \return The list of division points for the metrics.
///////////////////////////////////////////////////////////////////////////////
QString RideMap3DWindow::getRangeSettings()
{
    QString wReturning;
    RideWallFactory &wFactory = RideWallFactory::instance();

    // Go through each 3D wall.
    for (int i = 0; i < wFactory.count(); i++)
    {
        QString wRange;
        QString wName = wFactory[i]->getName();

        // Skip RideWall with no name of type DataSeries (none)
        if (wName.isEmpty() && (wFactory[i]->getType() == RideWallFactory::DATASERIES))
            continue;

        // Separate wall name from its colors list.
        wRange.append(wName + "@");

        // Create a string with the points' value.
        bool wFirstPass = true;
        if (wFactory[i]->isDefaultPoints() == false)
        {
            for (auto &wPointItr : wFactory[i]->pointsList())
            {
                // First point.
                if (wFirstPass)
                {
                    wFirstPass = false;
                    wRange.append(QString("%1").arg(wPointItr));
                }
                // Next point.
                else
                {
                    wRange.append(QString("; %1").arg(wPointItr));
                }
            }
        }

        // Walls separator character.
        wReturning.append(wRange + "%");
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::setUserData
///        This method loads the user data settings of the chart.
///
/// \param[in] iSettings    User data settings string.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::setUserData(QString iSettings)
{
    qDebug() << Q_FUNC_INFO;

    // Delete any UserData series.
    foreach(UserData *wUserDataItr, m_userDataSeries) delete wUserDataItr;
    m_userDataSeries.clear();

    // Snip into discrete user data xml snippets.
    QRegExp wSnippet("(\\<userdata .*\\<\\/userdata\\>)");
    wSnippet.setMinimal(true); // don't match too much
    QStringList wSnips;
    int wPos = 0;

    while ((wPos = wSnippet.indexIn(iSettings, wPos)) != -1) {
        wSnips << wSnippet.cap(1);
        wPos += wSnippet.matchedLength();
    }

    // Create and add each series.
    foreach(QString wSet, wSnips)
        m_userDataSeries << new UserData(wSet);

    // Fill with the current rideItem.
    setRideForUserData();

    // Update table.
    refreshCustomTable();

    // Update metric selection combobox.
    refreshMetricList(INIT_LIST);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::setMetricSelection
///        This method sets the metric selection index.
///
/// \param[in] iSelection   Selection index.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::setMetricSelection(const int iSelection)
{
    int wSelection = (iSelection < m_rideWallSelection->count()) ? iSelection : 0;
    m_rideWallSelection->setCurrentIndex(wSelection);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::setColorSettings
///        This method restores the saved configuration for the metrics.
///
/// \param[in] iColorsSettings  A string containing the walls colors' name.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::setColorSettings(QString iColorsSettings)
{
    // Get each wall colors list.
    QStringList wMetricWalls = iColorsSettings.split('%', QString::SkipEmptyParts);

    // Map colors for each 3D wall.
    QMap<QString, QList<QColor>> wMetricsColors;
    for (auto &wMetricItr : wMetricWalls)
    {
        QStringList wColorWalls = wMetricItr.split("@", QString::SkipEmptyParts);

        if (wColorWalls.count() <= 1)
            continue;

        // Get wall name.
        QString wWallName = wColorWalls[0];

        // Extract color strings.
        QStringList wColorsList = wColorWalls[1].split("; ", QString::SkipEmptyParts);

        // Create each color from their name.
        for (auto &wColorItr : wColorsList)
            wMetricsColors[wWallName].append(QColor(wColorItr));
    }

    // Set colors for each 3D wall.
    RideWallFactory &wFactory = RideWallFactory::instance();
    for (int i = 0; i < wFactory.count(); i++)
    {
        QString wName = wFactory[i]->getName();
        if (wMetricsColors.count(wName) > 0)
            wFactory[i]->setColorsList(wMetricsColors[wName]);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::setRangeSettings
///        This method restores the saved configuration for the metrics.
///
/// \param[in] iPointsSettings A string containing the division points' value.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::setRangeSettings(QString iPointsSettings)
{
    qDebug() << Q_FUNC_INFO;

    // Get each wall division points.
    QStringList wMetricWalls = iPointsSettings.split('%', QString::SkipEmptyParts);

    // Map division points for each 3D wall.
    QMap<QString, QList<double>> wMetricsPoints;
    for (auto &wMetricItr : wMetricWalls)
    {
        QStringList wPointWalls = wMetricItr.split("@", QString::SkipEmptyParts);

        if (wPointWalls.count() <= 1)
            continue;

        // Get wall's name.
        QString wWallName = wPointWalls[0];

        // Get wall's points list.
        QStringList wPointsList = wPointWalls[1].split("; ", QString::SkipEmptyParts);

        // Map division points as values.
        for (auto &wPointItr : wPointsList)
            wMetricsPoints[wWallName].append(wPointItr.toDouble());
    }

    // Set division points to each 3D wall.
    RideWallFactory &wFactory = RideWallFactory::instance();
    for (int i = 0; i < wFactory.count(); i++)
    {
        QString wName = wFactory[i]->getName();
        if (wMetricsPoints.count(wName) > 0)
            wFactory[i]->setPointsList(wMetricsPoints[wName]);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::getAltitude
///        This method gets the altitude at a specific time sample. The value
///        needs to be greater or equal to 0 if we don't want the wall to go
///        under the map.
///
/// \param[in] iTime        Timestamp.
/// \param[in] iRideItem    Current ride item.
///
/// \return The altitude value.
///////////////////////////////////////////////////////////////////////////////
double RideMap3DWindow::getAltitude(const double iTime, RideItem *iRideItem)
{
    double wReturning = 0.0;

    if (iRideItem && iRideItem->ride())
    {
        int wIndex = iRideItem->ride()->timeIndex(iTime);
        wReturning = std::max(0.0, iRideItem->ride()->dataPoints()[wIndex]->alt);
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::getDistance
///        This method gets the distance at a specific time sample.
///
/// \param[in] iTime        Timestamp.
/// \param[in] iRideItem    Current ride item.
///
/// \return The distance.
///////////////////////////////////////////////////////////////////////////////
double RideMap3DWindow::getDistance(const double iTime, RideItem* iRideItem)
{
    double wReturning = 0.0;

    if (iRideItem && iRideItem->ride())
    {
        int wIndex = iRideItem->ride()->timeIndex(iTime);
        wReturning = iRideItem->ride()->dataPoints()[wIndex]->km;
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::resolveInitdata
///        This method generates the 3D wall data used to be displayed on the
///        map.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::resolveInitdata()
{
    qDebug() << Q_FUNC_INFO;

    if (m_currentRideItem && m_currentRideItem->ride() && m_rideWallMethod && m_currentRideItem->ride()->dataPoints().count())
    {
        // Local javascript strings containing 3D wall data.
        QVector<s_map3DPoint> wIntervalDataMain;
        QVector<double> wIntervalDataExtra;

        int wStartIndex = 0;
        int wStopIndex = m_currentRideItem->ride()->dataPoints().count() - 1;

        // Reset min, max and wall validity.
        m_rideWallMethod->resetWallData();

        if (isUsingExtraData && m_extraDataWall)
            m_extraDataWall->resetWallData();

        for(int wIndex = wStartIndex; wIndex <= wStopIndex ; wIndex++)
        {
            // Skip sample if no longitude nor latitude.
            if(!(std::abs(m_currentRideItem->ride()->dataPoints()[wIndex]->lon) > 0.0) && !(std::abs(m_currentRideItem->ride()->dataPoints()[wIndex]->lat) > 0.0))
                continue;

            // Apply a modulo to separate ride into defined sections.
            double wIntPart = 0.0;
            double wFractPart = 0.0;
            double wTimestamp = m_currentRideItem->ride()->dataPoints()[wIndex]->secs;
            wFractPart = modf(wTimestamp, &wIntPart);

            // Append points for each section and for the last one.
            if (((wFractPart == 0.0) && (static_cast<int>(wIntPart) % m_sectionDuration->value() == 0)) || (wIndex == wStopIndex))
            {
                // Get the average value of the metric for the section.
                double wValue = m_rideWallMethod->getValue(wTimestamp, wTimestamp + m_sectionDuration->value(), m_currentRideItem);

                // Append point to the vector.
                s_map3DPoint wPointWall;
                wPointWall.mLon = m_currentRideItem->ride()->dataPoints()[wIndex]->lon;
                wPointWall.mLat = m_currentRideItem->ride()->dataPoints()[wIndex]->lat;
                wPointWall.mAlt = getAltitude(wTimestamp, m_currentRideItem);
                wPointWall.mDist = getDistance(wTimestamp, m_currentRideItem);
                wPointWall.mValue = wValue;
                wPointWall.mSecs = wTimestamp;

                wIntervalDataMain.append(wPointWall);

                // Extra Data.
                if (isUsingExtraData && m_extraDataWall)
                {
                    // Get the average value of the extra data metric for the section.
                    double wExtraValue = m_extraDataWall->getValue(wTimestamp, wTimestamp + m_sectionDuration->value(), m_currentRideItem);

                    // Append data to the vector.
                    wIntervalDataExtra.append(wExtraValue);
                }
            }
        }

        // A 3D wall needs 2 points for it to be displayed.
        m_intvlMainPoints = (wIntervalDataMain.count() > 1) ? wIntervalDataMain : QVector<s_map3DPoint>();
        m_intvlExtraPoints = (wIntervalDataExtra.count() > 1) ? wIntervalDataExtra : QVector<double>();

        // Create the 3D wall zones.
        m_rideWallMethod->buildZoneString();

        if (isUsingExtraData && m_extraDataWall)
            m_extraDataWall->buildZoneString();

        qDebug() << "RideMap3DWindow::resolveInitdata - Data computed.";
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::resolveJavaScript
///        This method resolves the javascript string containing the 3D wall
///        data. It then runs different scripts to initialize the data on the
///        Cesium server.
///
/// \param[in] iInitType Initialization type.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::resolveJavaScript(eInitType iInitType)
{
    qDebug() << Q_FUNC_INFO;

    // Try to lock function process.
    if (!m_javascriptRunning.tryLock(100))
        return;

    // Check if we can proceed in creating the javascript.
    if(m_currentRideItem && m_view && m_webBridge && m_rideWallMethod && m_intvlMainPoints.count())
    {
        // Build up the 3D wall data string for initialization.
        QString wDataForInit = QString("points = ['%1',[").arg(m_rideWallMethod->getName());
        for (auto &wPointItr : m_intvlMainPoints)
        {
            wDataForInit += wPointItr;
            wDataForInit += ",";
        }

        // Finalize the initialization data string.
        if (wDataForInit.endsWith(','))
            wDataForInit.chop(1);

        if (wDataForInit.endsWith("}]"))
            wDataForInit += "];";
        else
            wDataForInit += "]];";

        qDebug() << "DataForInit:\n" << wDataForInit << "\n";

        // Run javascript to initialize the main data.
        {
            m_view->page()->runJavaScript(wDataForInit, DataInitCallback(m_webBridge));
            QEventLoop wLoop;
            QTimer::singleShot(5000, &wLoop, SLOT(quit()));
            connect(m_webBridge, SIGNAL(DataInitCallbackDone()), &wLoop, SLOT(quit()));
            wLoop.exec(QEventLoop::WaitForMoreEvents);
        }

        // Get javascript zones string.
        QString wZones = m_rideWallMethod->getZoneColors();
        qDebug() << "Main wall Zones\n" << wZones;

        // Run javascript to apply the colors.
        {
            m_view->page()->runJavaScript(wZones, ZonesCallback(m_webBridge));
            QEventLoop wLoop;
            QTimer::singleShot(5000, &wLoop, SLOT(quit()));
            connect(m_webBridge, SIGNAL(ZonesCallbackDone()), &wLoop, SLOT(quit()));
            wLoop.exec(QEventLoop::WaitForMoreEvents);
        }

        // Initialize the map according to the data.
        if(iInitType == INIT_MAP3D)
        {
            m_view->page()->runJavaScript("init();", InitCallback(m_webBridge));
            QEventLoop wLoop;
            QTimer::singleShot(5000, &wLoop, SLOT(quit()));
            connect(m_webBridge, SIGNAL(InitCallbackDone()), &wLoop, SLOT(quit()));
            wLoop.exec(QEventLoop::WaitForMoreEvents);
        }
        // Refresh the data on the map.
        else if (iInitType == REINIT_DATA)
        {
            m_view->page()->runJavaScript("reInit();", ReInitCallback(m_webBridge));
            QEventLoop wLoop;
            QTimer::singleShot(5000, &wLoop, SLOT(quit()));
            connect(m_webBridge, SIGNAL(ReInitCallbackDone()), &wLoop, SLOT(quit()));
            wLoop.exec(QEventLoop::WaitForMoreEvents);
        }

        // Add extra Data after map initialization (add second 3D wall on top of the current one).
        if (isUsingExtraData && m_extraDataWall && (m_rideWallMethod != m_extraDataWall))
        {
            // Build the javascript string.
            QString wDataSetName = m_extraDataWall->getName();
            QString wAdditionalData = QString("additionalValue(['%1', [").arg(wDataSetName);

            for (auto &wPointItr : m_intvlExtraPoints)
            {
                wAdditionalData += QString::number(wPointItr);
                wAdditionalData += ",";
            }

            // Finalize the initialization data string.
            if (wAdditionalData.endsWith(','))
                wAdditionalData.chop(1);

            // Append extra 3D wall zones.
            wAdditionalData += "]], " + m_extraDataWall->getZoneColors();
            wAdditionalData += ");";

            qDebug() << "Extra Data string:\n" << wAdditionalData;

            // Run javascript.
            m_view->page()->runJavaScript(wAdditionalData, AddValueCallback(m_webBridge));
            QEventLoop wLoop;
            QTimer::singleShot(5000, &wLoop, SLOT(quit()));
            connect(m_webBridge, SIGNAL(AddValueCallbackDone()), &wLoop, SLOT(quit()));
            wLoop.exec(QEventLoop::WaitForMoreEvents);
        }
    }

    // Unlock mutex.
    m_javascriptRunning.unlock();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::displayAllActivity
///        This method displays the entire activity.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::displayAllActivity()
{
    qDebug() << Q_FUNC_INFO;

    // Try to lock mutex.
    if (m_javascriptRunning.tryLock(100) == false)
        return;

    m_view->page()->runJavaScript("displayAll()", DisplayAllCallback(m_webBridge));
    QEventLoop wLoop;
    QTimer::singleShot(5000, &wLoop, SLOT(quit()));
    connect(m_webBridge, SIGNAL(DisplayAllCallbackDone()), &wLoop, SLOT(quit()));
    wLoop.exec(QEventLoop::WaitForMoreEvents);

    m_javascriptRunning.unlock();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::displaySelectedIntervals
///        This method displays only the selected intervals from the 3D wall.
///        If the Entire Activity is selected, the entire wall will be
///        displayed.
///
///        To implement - Display only segments from the selected intervals
///        instead of all segments from the begining of first interval to the
///        end of last interval. It will require to add a functionality to the
///        API.
///
/// \param[in] iSelectedIntervals   Intervals to display.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::displaySelectedIntervals(QList<IntervalItem *> &iSelectedIntervals)
{
    // Get starting and ending 3D wall sections indexes for each interval.
    QVector<int> wMinIndexes, wMaxIndexes;
    for (auto &wIntervalItr : iSelectedIntervals)
    {
        int wStartIndex = 0;
        int wEndIndex = 0;

        getSelectedIntervalIndexes(wIntervalItr, wStartIndex, wEndIndex);
        wMinIndexes.append(wStartIndex);
        wMaxIndexes.append(wEndIndex);
    }

    // Try to lock mutex.
    if (m_javascriptRunning.tryLock(100) == false)
        return;

    // Get the minimum and maximum indexes.
    const int *wStartingAt = std::min_element(wMinIndexes.begin(), wMinIndexes.end());
    const int *wEndingAt = std::max_element(wMaxIndexes.begin(), wMaxIndexes.end());

    // Display the wall from the begining of the first interval to the end of the last interval.
    qDebug() << "Display segments from" << *wStartingAt << "to" << *wEndingAt;

    m_view->page()->runJavaScript(QString("displaySegment(%1, %2)").arg(QString::number(*wStartingAt), QString::number(*wEndingAt)), DisplaySegmentCallback(m_webBridge));
    QEventLoop wLoop;
    QTimer::singleShot(5000, &wLoop, SLOT(quit()));
    connect(m_webBridge, SIGNAL(DisplaySegmentCallbackDone()), &wLoop, SLOT(quit()));
    wLoop.exec(QEventLoop::WaitForMoreEvents);

    m_javascriptRunning.unlock();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::getSelectedIntervalIndexes
///        This method gets the 3D wall starting and ending indexes of a
///        specific interval.
///
/// \param[in]  iSelectedInterval   Specific interval.
/// \param[out] oStartIndex         Starting index output.
/// \param[out] oStopIndex          Ending index output.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::getSelectedIntervalIndexes(IntervalItem *iSelectedInterval, int &oStartIndex, int &oStopIndex)
{
    qDebug() << Q_FUNC_INFO;

    oStartIndex = oStopIndex = 0;

    // Get the 3D wall starting and ending indexes of an interval.
    if (iSelectedInterval && m_currentRideItem && m_currentRideItem->intervals(RideFileInterval::ALL).count())
    {
        IntervalItem *wEntireActivity = m_currentRideItem->intervals(RideFileInterval::ALL).at(0);

        if (wEntireActivity && m_intvlMainPoints.count())
        {
            // Initialize result to Entire Activity values.
            int wStartIndex = -1;
            int wEndIndex = -1;

            // Go through the Entire Activity to find interval indexes.
            int wLoopIndex = 0;
            for (auto &wPointItr : m_intvlMainPoints)
            {
                if (iSelectedInterval->start > wPointItr.mSecs)
                    wStartIndex = wLoopIndex;

                if (iSelectedInterval->stop > wPointItr.mSecs)
                    wEndIndex = wLoopIndex;
                else
                    break;

                wLoopIndex++;
            }

            // Validate results.
            oStartIndex = std::max(wStartIndex, 0);
            oStopIndex = std::max(oStartIndex, std::min(wEndIndex, (m_intvlMainPoints.count() - 1)));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::showFullPlotChanged
///        This method enables/disables the small plot under the Map 3D.
///
/// \param[in] iValue    Visibility state.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::showFullPlotChanged(const int iValue)
{
    m_smallPlot3D->setVisible(iValue != 0);
    m_showFullPlotAlt->setEnabled(static_cast<bool>(iValue));
    m_showFullPlotAltText->setEnabled(static_cast<bool>(iValue));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::showFullPlotAltChanged
///        This method enables/disables the altitude curve in the map 3D small
///        plot.
///
/// \param[in] iValue   Visibility state.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::showFullPlotAltChanged(const int iValue)
{
    m_smallPlot3D->setAltitudeVisible(static_cast<bool>(iValue));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::showIntvlOvlChanged
///        This method is called when the interval overlay check box state
///        changes to show or hide the overlay dialog.
///
/// \param[in] iValue   State of the checkbox.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::showIntvlOvlChanged(const int iValue)
{
    // Show or hide the helper.
    overlayWidget->setVisible(iValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::editUserData
///        This method opens the edit dialog box to edit a user data series.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::editUserData()
{
    QList<QTableWidgetItem*> wItems = m_customTable->selectedItems();
    if (wItems.count() < 1) return;

    int wIndex = m_customTable->row(wItems.first());

    UserData wEdit(m_userDataSeries[wIndex]->name,
                   m_userDataSeries[wIndex]->units,
                   m_userDataSeries[wIndex]->formula,
                   kZstringNotUsed,
                   m_userDataSeries[wIndex]->color);

    // Create the dialog.
    EditUserDataDialog wDialog(m_context, &wEdit);

    // Open the dialog.
    if (wDialog.exec())
    {
        // Apply!
        m_userDataSeries[wIndex]->formula = wEdit.formula;
        m_userDataSeries[wIndex]->units = wEdit.units;
        m_userDataSeries[wIndex]->color = wEdit.color;
        // zstring is not used for the 3D Map.

        // Check if it has been renamed.
        eUserDataChanged wNameChanged = (m_userDataSeries[wIndex]->name != wEdit.name) ? RENAMED : NO_ACTION;
        m_userDataSeries[wIndex]->name = wEdit.name;

        // Update table.
        refreshCustomTable();

        // Fill with the current rideItem.
        setRideForUserData();

        // Refresh metric selection list.
        refreshMetricList(wNameChanged, wIndex);

        // Signal that the User Data changed.
        emit userDataChanged(CONFIG_APPEARANCE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::doubleClicked
///        This method opens the edit dialog when a user data series has been
///        double clicked.
///
/// \param[in] iRow     Row number of the series.
/// \param[in] iColumn  Column where the action occured.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::doubleClicked(int iRow, int iColumn)
{
    Q_UNUSED(iColumn)

    UserData wEdit(m_userDataSeries[iRow]->name,
                   m_userDataSeries[iRow]->units,
                   m_userDataSeries[iRow]->formula,
                   kZstringNotUsed,
                   m_userDataSeries[iRow]->color);

    // Create dialog.
    EditUserDataDialog wDialog(m_context, &wEdit);

    // Open dialog.
    if (wDialog.exec())
    {
        // Apply!
        m_userDataSeries[iRow]->formula = wEdit.formula;
        m_userDataSeries[iRow]->units = wEdit.units;
        m_userDataSeries[iRow]->color = wEdit.color;
        // zstring is not used for the 3D Map.

        // Check if it has been renamed.
        eUserDataChanged wNameChanged = (m_userDataSeries[iRow]->name != wEdit.name) ? RENAMED : NO_ACTION;
        m_userDataSeries[iRow]->name = wEdit.name;

        // Update table.
        refreshCustomTable();

        // Fill with the current rideItem.
        setRideForUserData();

        // Refresh the metric selection list.
        refreshMetricList(wNameChanged, iRow);

        // Signal that the User Data was changed.
        emit userDataChanged(CONFIG_APPEARANCE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::addUserData
///        This method adds a new user data series to the chart.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::addUserData()
{
    // Creates a dialog window.
    UserData wAdd;
    wAdd.zstring = kZstringNotUsed;
    EditUserDataDialog wDialog(m_context, &wAdd);

    // Opens the edit dialog.
    if (wDialog.exec())
    {
        // Apply.
        m_userDataSeries.append(new UserData(wAdd.name, wAdd.units, wAdd.formula, QString(""), wAdd.color));

        // Refresh table.
        refreshCustomTable();

        // Fill with the current rideItem.
        setRideForUserData();

        // Refrest the metric selection list.
        refreshMetricList((m_userDataSeries.count() == 1) ? INIT_LIST : ADDED);

        // Signal that a first User Data series was added.
        if (m_userDataSeries.count() == 1)
            emit userDataChanged(CONFIG_APPEARANCE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::deleteUserData
///        This method remove a specific user data series.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::deleteUserData()
{
    // Get the selected series.
    QList<QTableWidgetItem*> wItems = m_customTable->selectedItems();
    if (wItems.count() < 1) return;

    // Get the index and pointer of the series.
    int wIndex = m_customTable->row(wItems.first());
    UserData *wDeleteme = m_userDataSeries[wIndex];

    // Remove from the list.
    m_userDataSeries.removeAt(wIndex);

    // Remove the 3D wall from the factory.
    RideWallFactory::instance().removeUserDataWall(wDeleteme);
    delete wDeleteme;

    // Refresh table.
    refreshCustomTable();

    // Refresh the metric selection list.
    refreshMetricList(REMOVED, wIndex);

    // Signal that the User Data was changed.
    emit userDataChanged(CONFIG_APPEARANCE);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::moveUserDataUp
///        This method moves the currently selected user data series up in the
///        list of the user data setting tab.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::moveUserDataUp()
{
    // Get the selected series.
    QList<QTableWidgetItem*> wItems = m_customTable->selectedItems();
    if (wItems.count() < 1) return;

    // Disable up button to give time to refresh metric list.
    m_upCustomButton->setEnabled(false);

    // Get the index of the series.
    int wIndex = m_customTable->row(wItems.first());

    // Move up if not the first item.
    if (wIndex > 0)
    {
        // Swap items.
        m_userDataSeries.swap(wIndex, wIndex - 1);

        // Refresh table.
        refreshCustomTable(wIndex - 1);

        // Refresh the metric selection list.
        refreshMetricList(MOVED_UP, wIndex);

        // Signal that the User Data was changed.
        emit userDataChanged(CONFIG_APPEARANCE);
    }

    // Enable the up button.
    m_upCustomButton->setEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::moveUserDataDown
///        This method moves the currently selected user data series down in
///        the list of the user data setting tab.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::moveUserDataDown()
{
    // Get the selected series.
    QList<QTableWidgetItem*> wItems = m_customTable->selectedItems();
    if (wItems.count() < 1) return;

    // Disable down button to give time to refresh metric list.
    m_downCustomButton->setEnabled(false);

    // Get the index of the series.
    int wIndex = m_customTable->row(wItems.first());

    // Move down if not the last item.
    if ((wIndex + 1) <  m_userDataSeries.size())
    {
        // Swap items.
        m_userDataSeries.swap(wIndex, wIndex+1);

        // Refresh table.
        refreshCustomTable(wIndex+1);

        // Refresh the metric selection list.
        refreshMetricList(MOVED_DOWN, wIndex);

        // Signal that the User Data was changed.
        emit userDataChanged(CONFIG_APPEARANCE);
    }

    // Enable the down button.
    m_downCustomButton->setEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::wallFactorySelectionChanged
///        This method is called when the metric selection changed.
///        This include automatic selection changed when ride selected and no
///        metric nor GPS data available.
///
/// \param[in] iIndex   The index of the metric to be displayed.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::wallFactorySelectionChanged(int iIndex)
{
    qDebug() << Q_FUNC_INFO;

    // Update the Settings window metric selection menu.
    if (m_rideWallSelectionSettings)
    {
        m_rideWallSelectionSettings->blockSignals(true);
        m_rideWallSelectionSettings->setCurrentIndex(iIndex);
        m_rideWallSelectionSettings->blockSignals(false);
    }

    // Set invalid wall by default.
    m_rideWallMethod = RideWallFactory::instance().getRideWall(RideFile::none);

    // Select the new ride wall to display.
    RideFile::seriestype wSeries = static_cast<RideFile::seriestype>(m_rideWallSelection->currentData(Qt::UserRole).toInt());

    // DataSeries.
    if (wSeries < RideFile::none)
    {
        m_rideWallMethod = RideWallFactory::instance().getRideWall(wSeries);
        m_smallPlot3D->setMetric(RideFile::symbolForSeries(wSeries));
    }
    // User Data Series.
    else if (wSeries > RideFile::none)
    {
        QString wSelectionName = m_rideWallSelection->currentText();
        for (auto &wCustomItr : m_userDataSeries)
        {
            if (wCustomItr->name == wSelectionName)
            {
                m_rideWallMethod = RideWallFactory::instance().getRideWall(wCustomItr);
                m_smallPlot3D->setMetric(wCustomItr);
                break;
            }
        }
    }
    else
    {
        m_smallPlot3D->setMetric(RideFile::symbolForSeries(RideFile::none));
    }

    // Load the data.
    resolveInitdata();

    // Reinitialize data on 3D map.
    if (m_mapLoaded)
        resolveJavaScript(REINIT_DATA);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::currentRideRefreshWall
///        This method updates the colors and the points of a the current ride
///        wall when changed manually from the chart settings window.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::currentRideRefreshWall()
{
    qDebug() << Q_FUNC_INFO;

    if (m_rideWallMethod && m_rideWallColorBar && m_rideWallSelection)
    {
        // Activate the reset button.
        if (m_resetDefaultButton)
            m_resetDefaultButton->setDisabled(m_rideWallMethod->isDefaultColors() && m_rideWallMethod->isDefaultPoints());

        // Redraw ride wall.
        wallFactorySelectionChanged(m_rideWallSelection->currentIndex());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::changeWallPoints
///        This method is called when color bar points have been updated. It
///        refreshes the current wall.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::changeWallPoints(const QList<double> iPointsList)
{
    if (m_rideWallMethod)
        m_rideWallMethod->setPointsList(iPointsList);

    currentRideRefreshWall();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::changeWallPoints
///        This method is called when color bar colors have been updated. It
///        refreshes the current wall.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::changeWallColors(const QList<QColor> iColorList)
{
    if (m_rideWallMethod)
        m_rideWallMethod->setColorsList(iColorList);

    currentRideRefreshWall();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::updateColorBar
///        This method updates the setting window color bar with the current
///        ride wall.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::refreshColorBar()
{
    qDebug() << Q_FUNC_INFO;

    // Check if the currently selected item is enabled.
    bool wCurrentMetricEna = false;
    auto wCurrentItem = qobject_cast<QStandardItemModel *>(m_rideWallSelection->model())->item(m_rideWallSelection->currentIndex());
    if (m_rideWallSelection && wCurrentItem)
        wCurrentMetricEna = wCurrentItem->isEnabled();

    // Retrieve Ride wall colors and points to set the color bar.
    if (m_rideWallColorBar && m_rideWallMethod)
    {
        // Hide color bar. We cannot calculate default values if the 3D wall is not computed.
        bool wSetHidden = ((wCurrentMetricEna == false) || m_isBlank);
        m_rideWallColorBar->setHidden(wSetHidden);

        // Update color bar if visible.
        if (!wSetHidden)
        {
            m_rideWallColorBar->blockSignals(true);
            m_rideWallColorBar->setColors(m_rideWallMethod->colorsList());
            m_rideWallColorBar->setPoints(m_rideWallMethod->pointsList());
            m_rideWallColorBar->blockSignals(false);
        }

        // Disable reset button current 3D wall has its default colors and default points or the color bar is hidden.
        m_resetDefaultButton->setDisabled((m_rideWallMethod->isDefaultColors() && m_rideWallMethod->isDefaultPoints()) || wSetHidden);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::rideSelected
///        This method updates the 3D map when a ride is selected.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::rideSelected()
{
    qDebug() << Q_FUNC_INFO;

    RideItem *wRideItem = myRideItem;

    if (wRideItem == nullptr) m_currentRideItem = nullptr;

    // Fill User Dataseries with the current rideItem.
    setRideForUserData();

    QList<IntervalItem*> wAllInterval;

    m_validRideData = true;
    m_isBlank = false;

    if (wRideItem && wRideItem->ride() && wRideItem->ride()->dataPoints().count())
    {
        // Check data is within the ride.
        if (!wRideItem->ride()->areDataPresent()->lat || !wRideItem->ride()->areDataPresent()->lon ||
                !wRideItem->ride()->areDataPresent()->alt)
        {
            m_validRideData = false;
            m_isBlank = true;
        }

        // Ride is valid and can be displayed.
        if(!m_isBlank && m_validRideData && amVisible())
        {
            if ((wRideItem != m_currentRideItem) || m_forceReplot)
            {
                if (m_forceReplot)
                {
                    m_forceReplot = false;
                }

                // Remember what we last plotted.
                m_currentRideItem = wRideItem;

                // Set data to the subplot.
                m_smallPlot3D->setData(wRideItem);

                // Route metadata ...
                setSubTitle(wRideItem->ride()->getTag("Route", tr("Route")));

                // Compute 3D wall data.
                resolveInitdata();

                // Initialize 3D map for the entire ride interval.
                if (m_mapLoaded)
                {
                    resolveJavaScript(INIT_MAP3D);

                    qDebug() << "3D map updated";

                    // Reset view on the selected ride.
                    resetView();
                }
            }
        }
    }
    else
    {
        m_isBlank = true;
        m_validRideData = false;
    }

    // Refresh metric list.
    refreshMetricList();

    // Set blank state.
    setIsBlank(m_isBlank);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::intervalSelected
///        This method displays selected intervals and load their data.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::intervalSelected()
{
    qDebug() << Q_FUNC_INFO;

    // Skip display if data already drawn or invalid.
    if (amVisible() && !m_isBlank && m_validRideData && m_mapLoaded && (m_currentRideItem != nullptr))
    {
        // Verify if there are selected intervals.
        QList<IntervalItem *> currentIntervals = m_currentRideItem->intervalsSelected();
        if(currentIntervals.isEmpty())
        {
            // Display the entire 3D.
            displayAllActivity();
        }
        else
        {
            // Display selected intervals.
            displaySelectedIntervals(currentIntervals);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::zoomInterval
///        This method zooms on a specific interval.
///
/// \param[in] iInterval    Interval to zoom in.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::zoomInterval(IntervalItem *iInterval)
{
    qDebug() << Q_FUNC_INFO;

    if (amVisible() && !m_isBlank && m_mapLoaded && m_validRideData && (m_currentRideItem != nullptr))
    {
        int wStartIndex = 0;
        int wEndIndex = 0;

        // Get 3D wall zones indexes for the interval.
        getSelectedIntervalIndexes(iInterval, wStartIndex, wEndIndex);

        // Try to lock mutex.
        if (m_javascriptRunning.tryLock(100) == false)
            return;

        // Zoom in.
        m_view->page()->runJavaScript(QString("zoomSegment(%1, %2)").arg(QString::number(wStartIndex), QString::number(wEndIndex)), ZoomSegmentCallback(m_webBridge));
        QEventLoop wLoop;
        QTimer::singleShot(5000, &wLoop, SLOT(quit()));
        connect(m_webBridge, SIGNAL(ZoomSegmentCallbackDone()), &wLoop, SLOT(quit()));
        wLoop.exec(QEventLoop::WaitForMoreEvents);

        m_javascriptRunning.unlock();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::resetView
///        This method resets the 3D map view to center the map on the entire
///        activity.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::resetView()
{
    qDebug() << Q_FUNC_INFO;

    // Don't try to reset view if map not loaded.
    if (!m_mapLoaded)
        return;

    // Try to lock function process.
    if (!m_javascriptRunning.tryLock(100))
        return;

    // Make sure to display entire activity. The javascript function "resetView()"
    // centers on what's displayed on the map.
    {
        m_view->page()->runJavaScript("displayAll()", DisplayAllCallback(m_webBridge));
        QEventLoop wLoop;
        QTimer::singleShot(5000, &wLoop, SLOT(quit()));
        connect(m_webBridge, SIGNAL(DisplayAllCallbackDone()), &wLoop, SLOT(quit()));
        wLoop.exec(QEventLoop::WaitForMoreEvents);
    }

    // Reset view.
    {
        m_view->page()->runJavaScript("resetView()", ResetViewCallback(m_webBridge));
        QEventLoop wLoop;
        QTimer::singleShot(5000, &wLoop, SLOT(quit()));
        connect(m_webBridge, SIGNAL(ResetViewCallbackDone()), &wLoop, SLOT(quit()));
        wLoop.exec(QEventLoop::WaitForMoreEvents);
    }

    m_javascriptRunning.unlock();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::forceReplot
///        This method forces to redraw map after the ride has been modified.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::forceReplot()
{
    // The ride has been modified, force a reload of the data.
    m_forceReplot = true;
    rideSelected();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::configChanged
///        This method is called when something changed in the app
///        configuration or in the user data series default color.
///
/// \param[in] iConfig  Changed configuration flag.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::configChanged(qint32 iConfig)
{
    qDebug() << Q_FUNC_INFO;

    // Appearance change.
    if (iConfig & CONFIG_APPEARANCE)
    {
        setProperty("color", GColor(CPLOTBACKGROUND));
#ifndef Q_OS_MAC
        m_intervalOverlay->setStyleSheet(TabView::ourStyleSheet());
#endif
        // Redraw ride wall.
        if (m_rideWallSelection)
            wallFactorySelectionChanged(m_rideWallSelection->currentIndex());

        // Update color bar from chart settings.
        refreshColorBar();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::loadMap3DView
///        This method loads the Cesium 3D map URL into the view.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::loadMap3DView()
{
    qDebug() << Q_FUNC_INFO;

    // Load URL.
    qDebug() << "RideMap3DWindow::loadMap3DView load URL";
    m_view->load(*m_cloudUrl);

    m_context->mainWindow->setEnabled(false);

    // Wait until done or timed out.
    QEventLoop wLoop;
    QTimer::singleShot(10000, &wLoop, SLOT(quit()));
    connect(m_view, SIGNAL(loadFinished(bool)), &wLoop, SLOT(quit()));

    wLoop.exec(QEventLoop::WaitForMoreEvents);

    qDebug() << "RideMap3DWindow::loadMap3DView load URL finished";

    m_context->mainWindow->setEnabled(true);

    forceReplot();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::loadMap3DFinished
///        This method updates the loading state of the map when it is done.
///
/// \param[in] iPageState   Page load state.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::loadMap3DFinished(bool iPageState)
{
    m_mapLoaded = iPageState;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::updateFrame
///        This method is called to update view page frame when WEBKIT is used.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::updateFrame()
{
    qDebug() <<Q_FUNC_INFO;

    // Deleting the web bridge seems to be the only way to
    // reset state between it and the webpage.
    if (m_webBridge)
        delete m_webBridge;

    m_webBridge = new Map3DWebBridge(m_context, this);

#ifndef NOWEBKIT
    m_view->page()->mainFrame()->addToJavaScriptWindowObject("m_webBridge", m_webBridge);
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::metricSettingsSelectionChanged
///        This method is called when the metric selection is changed with the
///        settings dialog window.
///
/// \param[in] iIndex   The metric index from the drop-down menu.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::metricSettingsSelectionChanged(int iIndex)
{
    // Set the main selection menu from the chart to the selected metric.
    m_rideWallSelection->blockSignals(true);
    m_rideWallSelection->setCurrentIndex(iIndex);
    wallFactorySelectionChanged(iIndex);
    m_rideWallSelection->blockSignals(false);

    refreshColorBar();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::resetMetricDefault
///        This method reset the default metric settings for the color bar.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::resetMetricDefault()
{
    // Set defaults and refresh color bar.
    if (m_rideWallMethod)
    {
        m_rideWallMethod->resetDefault();
        refreshColorBar();
    }
    currentRideRefreshWall();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::refreshCustomTable
///        This method refresh the User Data table displayed in the chart
///        settings window.
///
/// \param[in] iIndexSelectedItem    Index of the selected item.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::refreshCustomTable(int iIndexSelectedItem)
{
    // Clear then repopulate custom table settings to reflect
    // the current LTMSettings.
    m_customTable->clear();

    // Get headers back.
    QStringList wHeader = { tr("Name"), tr("Formula") };
    m_customTable->setHorizontalHeaderLabels(wHeader);

    QTableWidgetItem *wSelected = nullptr;

    // Now lets add a row for each metric.
    m_customTable->setRowCount(m_userDataSeries.count());
    int i = 0;
    foreach (UserData *wUserDataItr, m_userDataSeries)
    {
        QTableWidgetItem *wTableItem = new QTableWidgetItem();
        wTableItem->setText(wUserDataItr->name);
        wTableItem->setFlags(wTableItem->flags() & (~Qt::ItemIsEditable));
        m_customTable->setItem(i,0,wTableItem);

        wTableItem = new QTableWidgetItem();
        wTableItem->setText(wUserDataItr->formula);
        wTableItem->setFlags(wTableItem->flags() & (~Qt::ItemIsEditable));
        m_customTable->setItem(i,1,wTableItem);

        // Keep the selected item from previous step (relevant for moving up/down)
        if (iIndexSelectedItem == i)
        {
            wSelected = wTableItem;
        }
        i++;
    }

    if (wSelected)
    {
        m_customTable->setCurrentItem(wSelected);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::setRideForUserData
///        This method sets the current ride to the user data series.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::setRideForUserData()
{
    RideItem *wRide = myRideItem;

    if (wRide)
    {
        // User data needs refreshing
        foreach(UserData *wUserDataItr, m_userDataSeries) wUserDataItr->setRideItem(wRide);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::initializeMetricList
///        This method initializes the metric selection list with both type
///        of series (standard and user data).
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::initializeMetricList()
{
    qDebug() << Q_FUNC_INFO;

    // Block combo box signals during metric list refresh.
    m_rideWallSelection->blockSignals(true);
    m_rideWallSelectionSettings->blockSignals(true);

    // Clear any previous list.
    m_rideWallSelection->clear();
    m_rideWallSelectionSettings->clear();

    // Add first item when no metric is selected or available.
    QString wNoData = tr("Choose Metric");

    m_rideWallSelection->addItem(wNoData, RideFile::none);
    m_rideWallSelectionSettings->addItem(wNoData, RideFile::none);

    // It should not be selectable.
    qobject_cast<QStandardItemModel *>(m_rideWallSelection->model())->item(0)->setSelectable(false);
    qobject_cast<QStandardItemModel *>(m_rideWallSelectionSettings->model())->item(0)->setSelectable(false);

    // Add dataseries metrics.
    for (int i = RideFile::secs; i < RideFile::none; i++)
    {
        // Get 3D wall (create it if doesn't exist).
        RideFile::SeriesType wSeries = static_cast<RideFile::SeriesType>(i);
        RideWall *wRideWall = RideWallFactory::instance().getRideWall(wSeries);

        // Wall exists.
        if (wRideWall)
        {
            m_rideWallSelection->addItem(RideFile::seriesName(wSeries), wSeries);
            m_rideWallSelectionSettings->addItem(RideFile::seriesName(wSeries), wSeries);
        }
    }

    // User Data separator text.
    QString wUserDataSeparator = tr("--- User Data ---");

    // Add User Data series metrics if any.
    bool wNextSectionBegins = true;
    for (auto &wCustomItr : m_userDataSeries)
    {
        // Get 3D wall (create it if doesn't exist).
        RideWall *wRideWall = RideWallFactory::instance().getRideWall(wCustomItr);

        // Wall exists.
        if (wRideWall)
        {
            // Add selection separator before the first UserData series name.
            if (wNextSectionBegins)
            {
                wNextSectionBegins = false;
                m_rideWallSelection->addItem(wUserDataSeparator, RideFile::none);
                m_rideWallSelectionSettings->addItem(wUserDataSeparator, RideFile::none);
                qobject_cast<QStandardItemModel *>(m_rideWallSelection->model())->item(m_rideWallSelection->count() - 1)->setSelectable(false);
                qobject_cast<QStandardItemModel *>(m_rideWallSelectionSettings->model())->item(m_rideWallSelectionSettings->count() - 1)->setSelectable(false);
            }

            m_rideWallSelection->addItem(wCustomItr->name, RideFile::none + 1);
            m_rideWallSelectionSettings->addItem(wCustomItr->name, RideFile::none + 1);
        }
    }

    // Unblock combo box signals.
    m_rideWallSelection->blockSignals(false);
    m_rideWallSelectionSettings->blockSignals(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::refreshMetricList
///        This method updates the metric selection list composed by standard
///        dataseries and user data series.
///
/// \param[in] iUserDataChanged     User data series modification type.
/// \param[in] iUserDataIndex       User data series new index value.
///////////////////////////////////////////////////////////////////////////////
void RideMap3DWindow::refreshMetricList(const eUserDataChanged iUserDataChanged, const int iUserDataIndex)
{
    qDebug() << Q_FUNC_INFO;

    // Remember current index.
    const int wPreviousIndex = m_rideWallSelection->currentIndex();
    bool wClearSelection = false;

    // Verify metric lists size.
    if ((m_rideWallSelection->count() == 0) || (m_rideWallSelectionSettings->count() == 0) ||
            (m_rideWallSelection->count() != m_rideWallSelectionSettings->count()) || (iUserDataChanged == INIT_LIST))
    {
        // Need to be initialized first.
        initializeMetricList();
    }

    // Block combo box signal during metric list refresh.
    m_rideWallSelection->blockSignals(true);
    m_rideWallSelectionSettings->blockSignals(true);

    // User Data separator text.
    QString wUserDataSeparator = tr("--- User Data ---");
    int wSeparatorIndex = m_rideWallSelection->findText(wUserDataSeparator, Qt::MatchExactly);
    int wUserDataChangedIndex = wSeparatorIndex + 1 + iUserDataIndex;

    // Skip if no user data section detected.
    if (wSeparatorIndex > -1)
    {
        // Manage the user data series changes.
        switch (iUserDataChanged)
        {
        // Adding first UserData series cleared metric list.
        case INIT_LIST:
            // Reselect metric.
            m_rideWallSelection->setCurrentIndex(wPreviousIndex);
            m_rideWallSelectionSettings->setCurrentIndex(wPreviousIndex);
            break;
        // A user data series has been deleted.
        case REMOVED:
        {
            // Remove from the comboboxes.
            m_rideWallSelection->removeItem(wUserDataChangedIndex);
            m_rideWallSelectionSettings->removeItem(wUserDataChangedIndex);

            // No more User Data metrics, remove section.
            if (m_userDataSeries.isEmpty())
            {
                m_rideWallSelection->removeItem(wSeparatorIndex);
                m_rideWallSelectionSettings->removeItem(wSeparatorIndex);
                wClearSelection = true;
            }
        }
            break;
        // User data series order changed.
        case MOVED_UP:
        case MOVED_DOWN:
        {
            int wInsertIndex = ((iUserDataChanged == MOVED_UP) ? wUserDataChangedIndex : (wUserDataChangedIndex + 1));
            int wRemovedIndex = ((iUserDataChanged == MOVED_UP) ? (wUserDataChangedIndex - 1) : wUserDataChangedIndex);

            // Get removed item data.
            QString wItemMovedText = m_rideWallSelection->itemText(wRemovedIndex);
            QVariant wItemMovedData = m_rideWallSelection->itemData(wRemovedIndex, Qt::UserRole);

            // Remove item from the lists and reinsert it.
            m_rideWallSelection->removeItem(wRemovedIndex);
            m_rideWallSelection->insertItem(wInsertIndex, wItemMovedText, wItemMovedData);

            m_rideWallSelectionSettings->removeItem(wRemovedIndex);
            m_rideWallSelectionSettings->insertItem(wInsertIndex, wItemMovedText, wItemMovedData);

            // Current item moved, reselect it.
            if (wPreviousIndex == wRemovedIndex)
            {
                m_rideWallSelection->setCurrentIndex(wInsertIndex);
                m_rideWallSelectionSettings->setCurrentIndex(wInsertIndex);
            }
        }
            break;
        // New UserData series added.
        case ADDED:
        {
            // Get 3D wall (create it if doesn't exist).
            RideWall *wRideWall = RideWallFactory::instance().getRideWall(m_userDataSeries.last());

            // Wall exists.
            if (wRideWall)
            {
                m_rideWallSelection->addItem(m_userDataSeries.last()->name, RideFile::none + 1);
                m_rideWallSelectionSettings->addItem(m_userDataSeries.last()->name, RideFile::none + 1);
            }
        }
            break;
        // UserData series has been renamed.
        case RENAMED:
            if ((wUserDataChangedIndex < m_rideWallSelection->count()) && (iUserDataIndex < m_userDataSeries.count()) &&
                    (iUserDataIndex >= 0))
            {
                // Rename item name.
                m_rideWallSelection->setItemText(wUserDataChangedIndex, m_userDataSeries[iUserDataIndex]->name);
                m_rideWallSelectionSettings->setItemText(wUserDataChangedIndex, m_userDataSeries[iUserDataIndex]->name);
            }
            break;
        default:
            break;
        }
    }
    else {
        qDebug() << "There is no User Data Section";
    }

    bool wDataAvailable = false;

    // Go through all metrics.
    for (int i = 0; i < m_rideWallSelection->count(); i++)
    {
        auto wItem = qobject_cast<QStandardItemModel *>(m_rideWallSelection->model())->item(i);
        auto wItemSettings = qobject_cast<QStandardItemModel *>(m_rideWallSelectionSettings->model())->item(i);
        bool wSeriesPresent = false;
        bool wIsUserDataSeparator = false;

        if (m_context && m_context->rideItem() && m_context->rideItem()->ride())
        {
            // Get series type from item data.
            RideFile::seriestype wSeries = static_cast<RideFile::seriestype>(wItem->data(Qt::UserRole).toInt());

            // User data metrics. (All user data series have their series type as "RideFile::none + 1" in the item data.
            if (wSeries > RideFile::none)
            {
                // Find the matching UserData series.
                QList<UserData *>::iterator wUserDataItr = m_userDataSeries.begin();
                for (; wUserDataItr != m_userDataSeries.end(); wUserDataItr++)
                    if ((*wUserDataItr)->name == wItem->text())
                        break;

                // UserData series found.
                if (wUserDataItr != m_userDataSeries.end())
                {
                    // Validate data presence.
                    for (auto &wDataItr : (*wUserDataItr)->vector)
                        wSeriesPresent |= (std::abs(wDataItr) > 0);
                }
            }
            else if (wSeries == RideFile::none)
            {
                wIsUserDataSeparator = (wSeparatorIndex == i);
            }
            // Standard dataseries.
            else
            {
                wSeriesPresent = m_context->rideItem()->ride()->isDataPresent(wSeries);
            }
        }

        // Update item enable state.
        wItem->setEnabled(wSeriesPresent || wIsUserDataSeparator);
        wItemSettings->setEnabled(wSeriesPresent || wIsUserDataSeparator);

        // Indicate if there is at least one series with data.
        wDataAvailable |= wSeriesPresent;
    }

    // Update first item in the metric selection box.
    QString wFirstItemString = tr("Choose Metric");

    // No metric has data available.
    if (wDataAvailable == false)
    {
        wFirstItemString = tr("No Metric Data");

        wClearSelection = true;
    }
    else if (m_isBlank)
    {
        // Blank when no GPS data is found.
        wFirstItemString = tr("No GPS Data");
    }

    // Set first item text.
    qobject_cast<QStandardItemModel *>(m_rideWallSelection->model())->item(0)->setText(wFirstItemString);
    qobject_cast<QStandardItemModel *>(m_rideWallSelectionSettings->model())->item(0)->setText(wFirstItemString);

    // Make sure to unblock signals.
    m_rideWallSelection->blockSignals(false);
    m_rideWallSelectionSettings->blockSignals(false);

    // No Metric Data.
    if (wClearSelection)
        m_rideWallSelection->setCurrentIndex(0);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideMap3DWindow::event
///        This method is called when an event occurs. It filter for a "Resize
///        Event" and move main widget.
///
/// \param[in] iEvent   Event that occured.
///
/// \return The input event.
///////////////////////////////////////////////////////////////////////////////
bool RideMap3DWindow::event(QEvent *iEvent)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and
    // we wait for it to be set properly then put our helper widget on the RHS
    if (iEvent->type() == QEvent::Resize && geometry().width() != 100) {

        // Put somewhere nice on first show.
        if (m_firstShow)
        {
            m_firstShow = false;
            helperWidget()->move(mainWidget()->geometry().width() - static_cast<int>(275 * dpiXFactor), static_cast<int>(50 * dpiYFactor));
        }

        // If off the screen, move on screen
        if (helperWidget()->geometry().x() > geometry().width())
        {
            helperWidget()->move(mainWidget()->geometry().width() - static_cast<int>(275 * dpiXFactor), static_cast<int>(50 * dpiYFactor));
        }
    }
    return QWidget::event(iEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Map3DWebBridge::Map3DWebBridge
///        Constructor.
///
/// \param[in] iContext Context object.
/// \param[in] iMap3D   3D map window pointer.
///////////////////////////////////////////////////////////////////////////////
Map3DWebBridge::Map3DWebBridge(Context *iContext, RideMap3DWindow *iMap3D) : m_context(iContext), m_map3D(iMap3D)
{
    // Connect context signals.
    connect(m_context, SIGNAL(intervalsChanged()), SLOT(intervalsSelected()));
    connect(m_context, SIGNAL(intervalSelected()), SLOT(intervalsSelected()));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Map3DWebBridge::intervalsSelected
///        This method calls the intervalSelected from the Ride Map 3D Window
///        class.
///////////////////////////////////////////////////////////////////////////////
void Map3DWebBridge::intervalsSelected()
{
    RideItem *rideItem = m_map3D->property("ride").value<RideItem*>();

    if (rideItem)
        m_map3D->intervalSelected();
}
