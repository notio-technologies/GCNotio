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

#ifndef RIDEMAP3DWINDOW_H
#define RIDEMAP3DWINDOW_H

#include "View3DFactory.h"
#include "IntervalItem.h"
#include "Context.h"

#include <QWidget>
#include <QDialog>
#include <QMap>

#ifdef NOWEBKIT
#include <QWebEnginePage>
#include <QWebEngineView>
#else
#include <QtWebKit>
#include <QWebPage>
#include <QWebView>
#include <QWebFrame>
#endif

class RideItem;
class Context;
class QVBoxLayout;
class IntervalSummaryWindow;
class RideMap3DPlot;
class ColorZonesBar;
class RideWall;
class QCheckBox;

class Map3DWebBridge;
class ResetViewCallback;
class DisplayAllCallback;
class DisplaySegmentCallBack;
class DataInitCallback;
class ZoomSegmentCallback;
class ZoomToCallback;
class ZonesCallback;
class InitCallback;
class ReInitCallback;
class AddValueCallback;

///////////////////////////////////////////////////////////////////////////////
/// \brief The RideMap3DWindow class
///        This class defines the 3D map chart.
///////////////////////////////////////////////////////////////////////////////
class RideMap3DWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // Properties can be saved/restored/set by the layout manager
    Q_PROPERTY(bool showfullplot READ showFullPlot WRITE setFullPlot USER true)
    Q_PROPERTY(bool showfullplotalt READ showFullPlotAlt WRITE setFullPlotAlt USER true)
    Q_PROPERTY(bool showintervals READ showIntervals WRITE setShowIntervals USER true)
    Q_PROPERTY(int sectionDuration READ getSectionDuration WRITE setSectionDuration USER true)

    // Must be declared before colorSettings and rangeSettings.
    Q_PROPERTY(QString userData READ getUserData WRITE setUserData USER true)

    // Must be after userData because it will influence the selection on GC startup.
    Q_PROPERTY(int metricSelection READ getMetricSelection WRITE setMetricSelection USER true)

    // RideWall properties (Depend on userData and must be declared after)
    Q_PROPERTY(QString colorSettings READ getColorSettings WRITE setColorSettings USER true)
    Q_PROPERTY(QString rangeSettings READ getRangeSettings WRITE setRangeSettings USER true)

    RideMap3DWindow();  // default ctor

    struct  s_map3DPoint {

        double mLon = 0;
        double mLat = 0;
        double mAlt = 0;
        double mDist = 0;
        double mValue = 0;
        double mSecs = 0;
    };

    friend QString& operator+=(QString &out, const s_map3DPoint &iPoint);

    static const QString kMap3DUrl;

    // Extra Data flag set to false (To be implemented in chart settings dialog)
    static const bool isUsingExtraData;

    typedef enum { INIT_MAP3D, REINIT_DATA } eInitType;
    typedef enum { NO_ACTION, INIT_LIST, ADDED, REMOVED, RENAMED, MOVED_UP, MOVED_DOWN } eUserDataChanged;

    public:
        RideMap3DWindow(Context *);
        virtual ~RideMap3DWindow();

        // Chart Settings properties getters.
        bool showFullPlot() const { return (m_fullPlotCheckBox->checkState() == Qt::Checked); }
        bool showFullPlotAlt() const { return  (m_showFullPlotAlt->checkState() == Qt::Checked); }
        bool showIntervals() const { return m_showIntvlOvl->isChecked(); }
        int getSectionDuration() const { return m_sectionDuration->value(); }
        QString getUserData() const;
        int getMetricSelection() const { return m_rideWallSelection->currentIndex(); }
        QString getColorSettings();
        QString getRangeSettings();

        // Chart Settings properties setters.
        void setFullPlot(const bool iState) { if (iState) m_fullPlotCheckBox->setCheckState(Qt::Checked); else m_fullPlotCheckBox->setCheckState(Qt::Unchecked); }
        void setFullPlotAlt(const bool iState) { m_showFullPlotAlt->setCheckState((iState ? Qt::Checked : Qt::Unchecked)); }
        void setShowIntervals(const bool iState) { m_showIntvlOvl->setChecked(iState); }
        void setSectionDuration(const int iValue) { m_sectionDuration->setValue(iValue); }
        void setUserData(QString);
        void setMetricSelection(const int iSelection);
        void setColorSettings(QString iColorsList);
        void setRangeSettings(QString iPointsList);

        // 3D wall JavaScript methods.
        double getAltitude(const double iTime, RideItem* iRide);
        double getDistance(const double iTime, RideItem* iRide);
        void resolveInitdata();
        void resolveJavaScript(eInitType initType);
        void displayAllActivity();
        void displaySelectedIntervals(QList<IntervalItem *> &);
        void getSelectedIntervalIndexes(IntervalItem *, int &, int &);

    public slots:
        void showFullPlotChanged(const int);
        void showFullPlotAltChanged(const int);
        void showIntvlOvlChanged(const int);

        // User data config page
        void editUserData();
        void doubleClicked(int, int);
        void addUserData();
        void deleteUserData();
        void moveUserDataUp();
        void moveUserDataDown();

        void wallFactorySelectionChanged(int);
        void currentRideRefreshWall();
        void changeWallPoints(const QList<double>);
        void changeWallColors(const QList<QColor>);
        void refreshColorBar();

        void rideSelected();
        void intervalSelected();

        void zoomInterval(IntervalItem *);
        void resetView();
        void forceReplot();
        void configChanged(qint32);

    private slots:
        void loadMap3DView();
        void loadMap3DFinished(bool);
        void updateFrame();
        void metricSettingsSelectionChanged(int);
        void resetMetricDefault();

    signals:
        void userDataChanged(qint32);

protected:
        // User data tab.
        void refreshCustomTable(int indexSelectedItem = -1);
        void setRideForUserData();
        QWidget *m_customUserData = nullptr;
        QTableWidget *m_customTable = nullptr;
        QPushButton *m_editCustomButton = nullptr, *m_addCustomButton = nullptr, *m_deleteCustomButton = nullptr;
#ifndef Q_OS_MAC
        QToolButton *m_upCustomButton = nullptr, *m_downCustomButton = nullptr;
#else
        QPushButton *m_upCustomButton = nullptr, *m_downCustomButton = nullptr;
#endif

    private:
        void initializeMetricList();
        void refreshMetricList(const eUserDataChanged iUserDataChanged = NO_ACTION, const int iUserDataIndex = 0);
        bool event(QEvent *iEvent);

        QMutex m_javascriptRunning;

        QVector<s_map3DPoint> m_intvlMainPoints;
        QVector<double> m_intvlExtraPoints;

        // User data series widgets.
        QList<UserData *> m_userDataSeries;
        const QString kZstringNotUsed = tr("Not used for the 3D Map.");

        QUrl *m_cloudUrl = nullptr;

        QCheckBox *m_fullPlotCheckBox = nullptr, *m_showIntvlOvl = nullptr;
        QCheckBox *m_showFullPlotAlt = nullptr;
        QLabel *m_showFullPlotAltText = nullptr;
        QSpinBox *m_sectionDuration = nullptr;
        QComboBox *m_rideWallSelection = nullptr;
        QComboBox *m_rideWallSelectionSettings = nullptr;
        QPushButton *m_resetDefaultButton = nullptr;
        ColorZonesBar *m_rideWallColorBar = nullptr;

        QVBoxLayout *m_mainLayout = nullptr;
        RideItem *m_currentRideItem = nullptr;
        IntervalSummaryWindow *m_intervalOverlay = nullptr;

#ifdef NOWEBKIT
        QWebEngineView *m_view = nullptr;
#else
        QWebView *m_view = nullptr;
#endif

        RideWall *m_rideWallMethod = nullptr;

        // Proof of concept. The extra data layer is not displayed logically.
        // It will double the height of the 3D wall. The wall height represents
        // the altitude. The Cesium map should instead display the extra layer
        // with a fixed height or modify the height of each layer depending on
        // how many layer there are.
        // To be used, the flag "isUsingExtraData" need to be set and the "m_extraDataWall"
        // needs to point to a 3D wall from the RideWallFactory. To be implemented in chart
        // setting dialog.
        RideWall *m_extraDataWall = nullptr;

        Context *m_context = nullptr;
        Map3DWebBridge *m_webBridge = nullptr;
        RideMap3DPlot *m_smallPlot3D = nullptr;

        bool m_mapLoaded = false;
        bool m_validRideData = false;
        bool m_isBlank = true;
        bool m_forceReplot = false;
        bool m_firstShow = true;
};

#ifdef NOWEBKIT
///////////////////////////////////////////////////////////////////////////////
/// \brief The Map3DWebPage class
///        This class is used to trick the maps api into ignoring gestures by
///        pretending to be chrome.
///
///        See: http://developer.qt.nokia.com/forums/viewthread/1643/P15
///////////////////////////////////////////////////////////////////////////////
class Map3DWebPage : public QWebEnginePage
{
};
#else
class Map3DWebPage : public QWebPage
{
#if 0
    virtual QString userAgentForUrl(const QUrl&) const {
        return "Mozilla/5.0";
    }
#endif
};
#endif

///////////////////////////////////////////////////////////////////////////////
/// \brief The Map3DWebBridge class
///        This class does the bridge between the 3D map and the execution of
///        the javascript used to display the map.
///////////////////////////////////////////////////////////////////////////////
class Map3DWebBridge : public QObject
{
    Q_OBJECT

    private:
        Context *m_context = nullptr;
        RideMap3DWindow *m_map3D = nullptr;

    public:
        Map3DWebBridge(Context *iContext, RideMap3DWindow *iMap3D);

signals:
        void ResetViewCallbackDone();
        void DisplayAllCallbackDone();
        void DisplaySegmentCallbackDone();
        void ZoomSegmentCallbackDone();
        void ZoomToCallbackDone();
        void DataInitCallbackDone();
        void ZonesCallbackDone();
        void InitCallbackDone();
        void ReInitCallbackDone();
        void AddValueCallbackDone();

    public slots:
        void intervalsSelected();
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The ResetViewCallback class
///        This class defines the javascript callback for the action to reset
///        the map view.
///////////////////////////////////////////////////////////////////////////////
class ResetViewCallback
{
public:
    ResetViewCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->ResetViewCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The DisplayAllCallback class
///        This class defines the javascript callback for the action to display
///        the entire activity 3D wall.
///////////////////////////////////////////////////////////////////////////////
class DisplayAllCallback
{
public:
    DisplayAllCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->DisplayAllCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The DisplaySegmentCallback class
///        This class defines the javascript callback for the action to display
///        a part of the activity 3D wall.
///////////////////////////////////////////////////////////////////////////////
class DisplaySegmentCallback
{
public:
    DisplaySegmentCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->DisplaySegmentCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The ZoomSegmentCallback class
///        This class defines the javascript callback for the action to zoom on
///        sequential segment composing the 3D wall.
///////////////////////////////////////////////////////////////////////////////
class ZoomSegmentCallback
{
public:
    ZoomSegmentCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->ZoomSegmentCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The ZoomToCallback class
///        This class defines the javascript callback for the action to zoom on
///        specific segment of the 3D wall.
///////////////////////////////////////////////////////////////////////////////
class ZoomToCallback
{
public:
    ZoomToCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->ZoomToCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The DataInitCallback class
///        This class defines the javascript callback for the action to init
///        the map data.
/// ///////////////////////////////////////////////////////////////////////////
class DataInitCallback
{
public:
    DataInitCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->DataInitCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The ZonesCallback class
///        This class defines the javascript callback for the action to set
///        the map zones.
///////////////////////////////////////////////////////////////////////////////
class ZonesCallback
{
public:
    ZonesCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->ZonesCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The InitCallback class
///        This class defines the javascript callback for the action to init
///        the map with the currently selected data.
///////////////////////////////////////////////////////////////////////////////
class InitCallback
{
public:
    InitCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->InitCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The ReInitCallback class
///        This class defines the javascript callback for the action to reinit
///        the map.
///////////////////////////////////////////////////////////////////////////////
class ReInitCallback
{
public:
    ReInitCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->ReInitCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The AddValueCallback class
///        This class defines the javascript callback for the action to add
///        additional data to the 3D wall.
///////////////////////////////////////////////////////////////////////////////
class AddValueCallback
{
public:
    AddValueCallback(Map3DWebBridge *iParent) : m_parent(iParent) {}
    void operator()(const QVariant &iResult) {
        qDebug() << Q_FUNC_INFO << iResult.toString();

        if (m_parent)
            emit m_parent->AddValueCallbackDone();
    }
    Map3DWebBridge *m_parent = nullptr;
};

#endif // RIDEMAP3DWINDOW_H
