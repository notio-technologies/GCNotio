/*
 * CorrectBCVx.cpp
 *
 *  Created on: 5 nov. 2016
 *      Author: NotioKonnect
 */
#include "DataProcessor.h"
#include "LTMOutliers.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <iostream>
#include "IntervalItem.h"

#include <QTime>

using namespace std;

class CorrectBCVx;

///////////////////////////////////////////////////////////////////////////////
/// \brief The CorrectBCVxConfig class
///        This class defines the configuration widget used by the Preferences/
///        Options config panes for "Correct BCVx using Garmin" data processor.
///////////////////////////////////////////////////////////////////////////////
class CorrectBCVxConfig: public DataProcessorConfig {
    Q_DECLARE_TR_FUNCTIONS( CorrectBCVxConfig)

    friend class ::CorrectBCVx;
protected:

    QHBoxLayout *mlayout;

    QLabel *offsetLabel;
    QSpinBox *offsetEdit;
    QCheckBox *m_autoOffset = nullptr;
    QLabel *flagsLabel;
    QComboBox *flagsEdit;

public:
    // Constructor
    CorrectBCVxConfig(QWidget *parent) :
        DataProcessorConfig(parent) {

        HelpWhatsThis *help = new HelpWhatsThis(parent);
        parent->setWhatsThis(
                    help->getWhatsThisText(
                        HelpWhatsThis::MenuBar_Edit_FixBCVxUsingGarmin));

        mlayout = new QHBoxLayout(this);
        mlayout->setContentsMargins(0, 0, 0, 0);
        setContentsMargins(0, 0, 0, 0);

        offsetLabel = new QLabel(tr("Manual Offset:"));
        offsetLabel->setToolTip(tr("Manual time offset between Notio and Garmin data."));
        offsetEdit = new QSpinBox();
        offsetEdit->setRange(SHRT_MIN, SHRT_MAX);
        offsetEdit->setSuffix(QString(" ") + tr("secs"));

        m_autoOffset = new QCheckBox(tr("Auto Detect"));
        m_autoOffset->setToolTip(tr("Automatically find offset between Notio and Garmin data."));
        m_autoOffset->setWhatsThis(tr("This feature will find an offset between Notio and Garmin data. "
                                      "It is important to choose carefully the intervals you want to "
                                      "correct. If no offset is detected, the value set manually will "
                                      "be used."));

        flagsLabel = new QLabel(tr("Metric:"));
        flagsLabel->setToolTip(tr("Metrics to fix in the BCVx data."));
        flagsEdit = new QComboBox();
        flagsEdit->addItem(tr("Power"));
        flagsEdit->addItem(tr("Speed"));
        flagsEdit->addItem(tr("Both"));

        // Button to reset configuration to default values.
        QPushButton *wDefault = new QPushButton();

        wDefault->setIcon(QPixmap(":images/toolbar/refresh.png"));
        wDefault->setIconSize(QSize(20, 20));
        wDefault->setMaximumSize(QSize(25, 25));
        wDefault->setToolTip(tr("Reset to default values."));
        wDefault->setFocusPolicy(Qt::NoFocus);

        // Set default values on button clicked without saving.
        connect(wDefault, &QPushButton::clicked, this, [this]{
            resetDefault();
        });

        QHBoxLayout *wButtonLayout = new QHBoxLayout();
        wButtonLayout->addWidget(wDefault);

        mlayout->addWidget(offsetLabel);
        mlayout->addWidget(offsetEdit);
        mlayout->addWidget(m_autoOffset);
        mlayout->addWidget(flagsLabel);
        mlayout->addWidget(flagsEdit);
        mlayout->addLayout(wButtonLayout);
        mlayout->addStretch();
    }

    //~CorrectBCVxConfig() {} // deliberately not declared since Qt will delete
    // the widget and its children when the config pane is deleted

    // Descriptive text within the configuration dialog window.
    QString explain() {
        return QString(tr("Power and/or Speed drops correction using Garmin data for "
                          "selected intervals.\n\n"
                          "1. Provide a time offset to Garmin file. For Garmin's data "
                          "ahead of Notio, a negative offset will shift Garmin right "
                          "to align them.\n\n"
                          "2. Select which metric to fix.\n"
                          "Note: Speed from Garmin data could be imprecise if coming "
                          "from the GPS instead of a speed sensor."));
    }

    // Read data processor parameters.
    void readConfig() {
        offsetEdit->setValue(appsettings->value(nullptr, GC_NOTIO_BCVX_OFFSET, -5).toInt());
        flagsEdit->setCurrentIndex(appsettings->value(nullptr, GC_NOTIO_BCVX_FLAGS, 1).toInt() - 1);    // Default power only.
        m_autoOffset->setChecked(appsettings->value(nullptr, GC_NOTIO_BCVX_AUTO, true).toBool());
    }

    // Save data processor parameters.
    void saveConfig() {
        appsettings->setValue(GC_NOTIO_BCVX_OFFSET, offsetEdit->value());
        appsettings->setValue(GC_NOTIO_BCVX_FLAGS, flagsEdit->currentIndex() + 1);
        appsettings->setValue(GC_NOTIO_BCVX_AUTO, m_autoOffset->isChecked());
    }

    // Reset data processor parameters to default.
    void resetDefault() {
        offsetEdit->setValue(-5);
        flagsEdit->setCurrentIndex(0);
        m_autoOffset->setChecked(true);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The s_peak struct
///        This stucture defines a peak used to find a time offset based on a
///        metric that two XDataSeries have in common.
///
/// \var    bool    isMin           Indicate a minimum peak else it is a maximum.
/// \var    int     xIndex          XDataPoint array index.
/// \var    double  x               x value representing the time in secs.
/// \var    double  y               y value of the peak.
/// \var    int[2]  range           Array index range to calculate integral.
/// \var    double  integralValue   Integral value of the peak within the range.
///////////////////////////////////////////////////////////////////////////////
struct s_peak {
    bool isMin = true;
    int xIndex = 0;
    double x = 0.0;
    double y = 0.0;
    int range[2] = { 0, 0 };
    double integralValue = 0.0;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The CorrectBCVx class
///        This class defines a data processor used to correct power and speed
///        lost in BCVx XData series.
///////////////////////////////////////////////////////////////////////////////
class CorrectBCVx: public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS( CorrectBCVx)

public:
    CorrectBCVx() {
    }
    ~CorrectBCVx() {
    }

    // The processor
    bool postProcess(RideFile *ride, DataProcessorConfig *config, QString op);

    // The config widget
    DataProcessorConfig* processorConfig(QWidget *parent) {
        return new CorrectBCVxConfig(parent);
    }

    // Localized Name
    QString name() {
        return (tr("Correct BCVx using Garmin"));
    }

    enum eFlagType { Power = 1, Speed = 2 };
    enum eOffsetType { Manual = 0, Auto };
    static constexpr int cErrorOffset = -9999;
    static int constexpr cPowerStatus = 0b00000000000000000000000000100000;
    static int constexpr cSpeedStatus = 0b00000000000000000000000000010000;

private:
    int fixBCVx(RideFile *ride, int iStart, int iEnd, int &ioOffset, const bool iAuto, const unsigned int iFlag);
    int estimateGarminOffset(XDataSeries *iSource1, XDataSeries *iSource2, const int iIntervalStart, const int iIntervalEnd);
    double averageValue(QVector<XDataPoint *> &iSource, int iIndex);
    double maxValue(QVector<XDataPoint *> &iSource, int iIndex);
    double minValue(QVector<XDataPoint *> &iSource, int iIndex);
    QVector<s_peak> findPeaks(QVector<XDataPoint *> &iSource, const int iIndex, const double iFactor = 1.0);
    QVector<QPair<s_peak, s_peak>> matchPeaks(QVector<s_peak> &iSource1Peaks, QVector<s_peak> &iSource2Peaks, const double iIntervalLength);
    static QString secsToString(const double iSecs) { return QTime(0,0,0,0).addSecs(static_cast<int>(iSecs)).toString("hh:mm:ss"); }

    // qSort function to sort peaks chronologically.
    static bool byTimeStamp(const s_peak &a, const s_peak &b) { return (a.x < b.x); }
};

// Register data processor.
static bool CorrectBCVxAdded =
        DataProcessorFactory::instance().registerProcessor(
            QString("_Correct BCVx"), new CorrectBCVx());

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::fixBCVx
///        This method is called by the processor and is used to fill the
///        missing data (power and speed drops) in the BCVx XData series using
///        Garmin data.
///
/// \param[in/out]  ride        Current RideFile object.
/// \param[in]      iStart      Interval start time.
/// \param[in]      iEnd        Interval stop time.
/// \param[in/out]  ioOffset    Time offset to apply to Garmin.
/// \param[in]      iAuto       Estimate automatically the time offset.
/// \param[in]      iFlag       Metrics selection flag.
///
/// \return The status of the operation. (User defined, Automatic or Error)
///////////////////////////////////////////////////////////////////////////////
int CorrectBCVx::fixBCVx(RideFile *ride, int iStart, int iEnd, int &ioOffset, const bool iAuto, const unsigned int iFlag)
{
    bool wBCVxModified = false;

    // Get the BCVx XData series.
    XDataSeries *wBCVxSeries = ride->xdata("BCVX");

    // Return if no data available.
    if ( wBCVxSeries == nullptr ) {
        cout << "No BCVx data" << endl;
        return cErrorOffset;
    }

    // Get the Garmin XData series.
    XDataSeries *wGarminSeries = ride->xdata("GARMIN");

    // Return if no data available.
    if ( wGarminSeries == nullptr ) {
        cout << "No Garmin data" << endl;
        return cErrorOffset;
    }

    // The interval must have at least 2 samples.
    if ((iEnd - iStart) < 2)
    {
        qDebug() << "Interval length is too short.";
        return cErrorOffset;
    }

    // BCVx data indexes.
    int statusByteIndex = wBCVxSeries->valuename.indexOf("statusByte");

    // If there is no status byte column, no point to continue.
    if (statusByteIndex < 0)
    {
        return cErrorOffset;
    }

    int BCVxPowerIndex = wBCVxSeries->valuename.indexOf("power");
    int BCVxSpeedIndex = wBCVxSeries->valuename.indexOf("speed");

    // Garmin data indexes.
    int GarminPowerIndex = wGarminSeries->valuename.indexOf("power");
    int GarminSpeedIndex = wGarminSeries->valuename.indexOf("speed");

    int garminPoints = wGarminSeries->datapoints.count();

    // Dertermine BCVx sample rate.
    XDataPoint* xpoint1 = wBCVxSeries->datapoints.at(0);
    XDataPoint* xpoint2 = wBCVxSeries->datapoints.at(1);
    double BCVxSampleRate = (xpoint2->secs - xpoint1->secs);

    cout << "SampleRate " << BCVxSampleRate << endl;

    if ((BCVxSampleRate > 0) == false)
        BCVxSampleRate = 1.0;

    // Determine Garmin sample rate.
    xpoint1 = wGarminSeries->datapoints.at(0);
    xpoint2 = wGarminSeries->datapoints.at(1);
    double GarminSampleRate = (xpoint2->secs - xpoint1->secs);

    cout << "Garmin SampleRate " << GarminSampleRate << endl;

    if ((GarminSampleRate > 0) == false)
        GarminSampleRate = 1.0;

    // Estimate automatically the time offset.
    int wEstOffset = (iAuto == true) ? estimateGarminOffset(wGarminSeries, wBCVxSeries, iStart, iEnd) : cErrorOffset;

    // Apply the estimated offset or the user defined offset.
    if (wEstOffset != cErrorOffset)
        ioOffset = wEstOffset;

    // Determine the start and then end vector indexes of the interval.
    int startPoint = static_cast<int>(iStart / BCVxSampleRate);
    int endPoint = static_cast<int>(iEnd / BCVxSampleRate);

    // Fix power drops.
    if (((iFlag & eFlagType::Power) > 0) && (BCVxPowerIndex >= 0) && (GarminPowerIndex >= 0)) {
        for (int i = startPoint; (i < endPoint) && (i < wBCVxSeries->datapoints.count()); i++) {
            XDataPoint* xpoint = wBCVxSeries->datapoints.at(i);
            double power = xpoint->number[BCVxPowerIndex];
            uint16_t statusByte  = static_cast<uint16_t>(xpoint->number[statusByteIndex]);

            // Check if there is no power value and if the power meter connection status is disconnected.
            if (((power > 0.0) == false) && ((statusByte & cPowerStatus) == 0)) {
                cout << "Power = 0 at " << i << endl;
                int use = static_cast<int>((i * BCVxSampleRate / GarminSampleRate) + ioOffset);
                if ( use >= 0 && use < garminPoints ) {
                    XDataPoint* xpoint2 = wGarminSeries->datapoints.at(use);
                    double garminPower = xpoint2->number[GarminPowerIndex];
                    cout << "Replace with " << garminPower << " at " << use << endl;
                    ride->command->setXDataPointValue("BCVX", i, BCVxPowerIndex + 2, garminPower );

                    // File has been modified.
                    wBCVxModified = true;
                }
            }
        }
    }

    // Fix speed drops.
    if (((iFlag & eFlagType::Speed) > 0) && (GarminSpeedIndex >= 0) && (BCVxSpeedIndex >= 0)) {

        int needToCorrect = 0;
        for (int i = startPoint; (i < endPoint) && (i < wBCVxSeries->datapoints.count()); i++) {

            XDataPoint* xpoint = wBCVxSeries->datapoints.at(i);
            uint16_t statusByte  = static_cast<uint16_t>(xpoint->number[statusByteIndex]);

            needToCorrect--;

            // Check if the speed sensor connection status is disconnected.
            if ( (statusByte & cSpeedStatus) == 0 ) {
                needToCorrect = 4;
            }

            if ( needToCorrect > 0 ) {
                int use = static_cast<int>((i  * BCVxSampleRate / GarminSampleRate) + ioOffset);
                if ( use >= 0 && use < garminPoints ) {
                    XDataPoint* xpoint2 = wGarminSeries->datapoints.at(use);

                    // Get and convert Garmin speed in km/h.
                    double garminSpeed = xpoint2->number[GarminSpeedIndex] * 3.6;   // Garmin have its speed in m/s.
                    cout << "Replace with " << garminSpeed << " at " << use << endl;
                    ride->command->setXDataPointValue("BCVX", i, BCVxSpeedIndex + 2, garminSpeed );

                    // File has been modified.
                    wBCVxModified = true;
                }
            }
        }
    }

    // Return if the automatic offset has been applied or else the user defined or if nothing appended.
    return (wBCVxModified ? ((wEstOffset == cErrorOffset) ? CorrectBCVx::Manual : CorrectBCVx::Auto) : cErrorOffset);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::estimateOffset
///        This method estimates automatically the time offset between two
///        XDataSeries for an interval.
///
/// \param[in]  iSource1        First XDataSeries source.
/// \param[in]  iSource2        Second XDataSeries source.
/// \param[in]  iIntervalStart  Start index of the interval.
/// \param[in]  iIntervalEnd    End index of the interval.
///
/// \return The estimated offset.
///////////////////////////////////////////////////////////////////////////////
int CorrectBCVx::estimateGarminOffset(XDataSeries *iSource1, XDataSeries *iSource2, const int iIntervalStart, const int iIntervalEnd)
{
    // Returning offset value initialized to error (no offset found)
    int wReturning = cErrorOffset;

    // Get power and speed indexes for both sources.
    int wPowerIndex1 = iSource1->valuename.indexOf("power");
    int wSpeedIndex1 = iSource1->valuename.indexOf("speed");
    int wPowerIndex2 = iSource2->valuename.indexOf("power");
    int wSpeedIndex2 = iSource2->valuename.indexOf("speed");

    // Exit if no data available.
    if (iSource1->datapoints.isEmpty() || iSource2->datapoints.isEmpty() || (wPowerIndex1 < 0) || (wPowerIndex2 < 0) || (wSpeedIndex1 < 0) || (wSpeedIndex2 < 0))
        return wReturning;

    // Get source 1 sample rate.
    double wSampleRate1 = iSource1->datapoints.at(1)->secs - iSource1->datapoints.at(0)->secs;
    if (wSampleRate1 < 0.0)
        wSampleRate1 = 1.0;

    // Get source 2 sample rate.
    double wSampleRate2 = iSource2->datapoints.at(1)->secs - iSource2->datapoints.at(0)->secs;
    if (wSampleRate2 < 0.0)
        wSampleRate2 = 1.0;

    // Get start and end indexes based on the sample rate for each source.
    int wStartPoint1 = static_cast<int>(iIntervalStart / wSampleRate1);
    int wEndPoint1 = static_cast<int>(iIntervalEnd / wSampleRate1);
    int wStartPoint2 = static_cast<int>(iIntervalStart / wSampleRate2);
    int wEndPoint2 = static_cast<int>(iIntervalEnd / wSampleRate2);

    // Determine a factor because the speed from Garmin data is in m/s.
    double wFactor1 = iSource1->name.contains("garmin", Qt::CaseInsensitive) ? 3.6 : 1;
    double wFactor2 = iSource2->name.contains("garmin", Qt::CaseInsensitive) ? 3.6 : 1;

    QVector<QPair<s_peak, s_peak>> wMatchingPeaks;
    QVector<XDataPoint *> wSource1DataPoints = iSource1->datapoints.mid(wStartPoint1, wEndPoint1 - wStartPoint1);
    QVector<XDataPoint *> wSource2DataPoints = iSource2->datapoints.mid(wStartPoint2, wEndPoint2 - wStartPoint2);
    double wIntervalLength = wSource1DataPoints.isEmpty() ? 0 : wSource1DataPoints.last()->secs - wSource1DataPoints.first()->secs;

    // Get matching Speed peaks between both sources.
    qDebug() << "Find" << iSource1->name << "speed peaks";
    QVector<s_peak> wSource1Peaks = findPeaks(wSource1DataPoints, wSpeedIndex1, wFactor1);

    qDebug() << "Find" << iSource2->name << "speed peaks";
    QVector<s_peak> wSource2Peaks = findPeaks(wSource2DataPoints, wSpeedIndex2, wFactor2);

    qDebug() << "Matching speed peaks for" << iSource1->name << "and" << iSource2->name;
    wMatchingPeaks.append(matchPeaks(wSource1Peaks, wSource2Peaks, wIntervalLength));

    wSource1Peaks.clear();
    wSource2Peaks.clear();

    // Get matching Power peaks between both sources.
    qDebug() << "Find" << iSource1->name << "power peaks";
    wSource1Peaks = findPeaks(wSource1DataPoints, wPowerIndex1);

    qDebug() << "Find" << iSource2->name << "power peaks";
    wSource2Peaks = findPeaks(wSource2DataPoints, wPowerIndex2);

    qDebug() << "Matching power peaks for" << iSource1->name << "and" << iSource2->name;
    wMatchingPeaks.append(matchPeaks(wSource1Peaks, wSource2Peaks, wIntervalLength));

#ifdef GC_DEBUG
    for (auto &wDebug : wMatchingPeaks)
    {
        qDebug() << "Matching peaks:";
        qDebug() << "Peak" << ((wDebug.first.isMin == true) ? "min" : "max") << iSource1->name << secsToString(wDebug.first.x) << "Metric Value:" << wDebug.first.y << "Integral value:" << wDebug.first.integralValue;
        qDebug() << "Peak" << ((wDebug.second.isMin == true) ? "min" : "max") << iSource2->name << secsToString(wDebug.second.x) << "Metric Value:" << wDebug.second.y << "Integral value:" << wDebug.second.integralValue;
        qDebug() << "Offset:" << static_cast<int>(wDebug.second.x - wDebug.first.x);
    }
#endif

    // Create a vector containing time offset between paired peaks.
    QVector<double> wPeaksTimeOffset;
    for (auto &wPeaksItr : wMatchingPeaks)
    {
        // Get Garmin data offset relative to BCVx data.
        if ((iSource1->name == "BCVX") && (iSource2->name == "GARMIN"))
            wPeaksTimeOffset.append(static_cast<int>(wPeaksItr.second.x - wPeaksItr.first.x));
        else if ((iSource1->name == "GARMIN") && (iSource2->name == "BCVX"))
            wPeaksTimeOffset.append(static_cast<int>(wPeaksItr.first.x - wPeaksItr.second.x));
    }

    double wAvgTimeOffset = 0.0, wStdDeviation = 0.0;

    int wCount = 0, wOriginalCnt = 0;
    double remStdDeviation = appsettings->value(nullptr, GC_NOTIO_BCVX_MATCH_PEAK_STD_DEV, 1.5).toDouble();

    // Skimm the matching peaks offset values.
    do {
        wOriginalCnt = wPeaksTimeOffset.count();

        // Calculate average offset.
        wAvgTimeOffset = std::accumulate(wPeaksTimeOffset.begin(), wPeaksTimeOffset.end(), .0) / wPeaksTimeOffset.size();

        // Calculate standard deviation.
        wStdDeviation = sqrt(std::accumulate(wPeaksTimeOffset.begin(), wPeaksTimeOffset.end(), 0.0, [&wAvgTimeOffset](const double &iAcc, const double &iValue) {
            return iAcc + pow(iValue - wAvgTimeOffset, 2);
        }) / (wPeaksTimeOffset.count()));

        // Remove values outside the range defined by the standard deviation and the average.
        wPeaksTimeOffset.erase(std::remove_if(wPeaksTimeOffset.begin(), wPeaksTimeOffset.end(), [&wStdDeviation, &wAvgTimeOffset, &remStdDeviation] (double i) {
            return ((i > (wAvgTimeOffset + remStdDeviation * wStdDeviation)) || (i < (wAvgTimeOffset - remStdDeviation * wStdDeviation)));
        }), wPeaksTimeOffset.end());

        wCount = wPeaksTimeOffset.count();

    } while ((wOriginalCnt != wCount) && (wCount != 0));

    qDebug() << "Standard Deviation:" << wStdDeviation;

    // Remove single occurences for a standard deviation passed a certain value.
    if (wStdDeviation > remStdDeviation)
    {
        wPeaksTimeOffset.erase(std::remove_if(wPeaksTimeOffset.begin(), wPeaksTimeOffset.end(), [&wPeaksTimeOffset] (double i) {
            int wCountNb = wPeaksTimeOffset.count(i);
            return (wCountNb < 2);
        }), wPeaksTimeOffset.end());

        // Recalculate average offset.
        wAvgTimeOffset = std::accumulate(wPeaksTimeOffset.begin(), wPeaksTimeOffset.end(), .0) / wPeaksTimeOffset.size();
    }

    qDebug() << "New offset list" << wPeaksTimeOffset;

    // No time offset found.
    if (wPeaksTimeOffset.isEmpty())
    {
        qDebug() << "Time offset not found";
    }
    else
    {
        wReturning = static_cast<int>(round(wAvgTimeOffset));
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::averageValue
///        This method finds the average value for a metric in a XDataPoint
///        vector.
///
/// \param[in] iSource  Source XDatapoints.
/// \param[in] iIndex   Index of the metric.
///
/// \return The average value.
///////////////////////////////////////////////////////////////////////////////
double CorrectBCVx::averageValue(QVector<XDataPoint *> &iSource, int iIndex)
{
    // Lambda function calculating the metric sum points.
    auto sumXDataPointSeries = [&iIndex](double iSum, XDataPoint *iPoint)
    {
        return iSum + iPoint->number[iIndex];
    };

    // Calculate the average.
    return std::accumulate(iSource.begin(), iSource.end(), 0.0, sumXDataPointSeries) / iSource.count();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::maxValue
///        This method finds the maximum value for a metric in a XDataPoint
///        vector.
///
/// \param[in] iSource  Source XDatapoints.
/// \param[in] iIndex   Index of the metric.
///
/// \return The maximum value.
///////////////////////////////////////////////////////////////////////////////
double CorrectBCVx::maxValue(QVector<XDataPoint *> &iSource, int iIndex)
{
    double wMaxValue = 0.0;

    // Get the XDataPoint with the highest value.
    auto wXDataPoint = *std::max_element(iSource.begin(), iSource.end(), [&iIndex](auto a, auto b)
    {
        return (b->number[iIndex] > a->number[iIndex]);
    });

    // Get the point's value.
    if (wXDataPoint)
        wMaxValue = std::max(wXDataPoint->number[iIndex], wMaxValue);

    return wMaxValue;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::minValue
///        This method finds the minimum value for a metric in a XDataPoint
///        vector.
///
/// \param[in] iSource  Source XDatapoints.
/// \param[in] iIndex   Index of the metric.
///
/// \return The minimum value.
///////////////////////////////////////////////////////////////////////////////
double CorrectBCVx::minValue(QVector<XDataPoint *> &iSource, int iIndex)
{
    // Get the maximum value.
    double wMinValue = maxValue(iSource, iIndex);

    // Get the XDataPoint with the lowest value.
    auto wXDataPoint = *std::min_element(iSource.begin(), iSource.end(), [&iIndex](auto a, auto b)
    {
        return (a->number[iIndex] < b->number[iIndex]);
    });

    // Get the point's value.
    if (wXDataPoint)
        wMinValue = std::min(wXDataPoint->number[iIndex], wMinValue);

    return wMinValue;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::findPeaks
///        This method finds minimum and maximum peak values from a metric
///        array of a XDataPoint vector.
///
/// \param[in] iSource      Source XDatapoints vector reference.
/// \param[in] iIndex       Index of the metric.
/// \param[in] iFactor      Factor to apply to the metric value.
///
/// \return A vector with a list of peaks.
///////////////////////////////////////////////////////////////////////////////
QVector<s_peak> CorrectBCVx::findPeaks(QVector<XDataPoint *> &iSource, const int iIndex, const double iFactor)
{
    QVector<s_peak> wReturning, wMaxPeaks, wMinPeaks;

    // Nothing to process.
    if (iSource.isEmpty())
        return wReturning;

    // Get the sourc sample rate.
    double wSampleRate = iSource.at(1)->secs - iSource.at(0)->secs;
    if ((wSampleRate > 0.0) == false)
        wSampleRate = 1.0;

    // Number of points to determine the range used for peak analysis.
    int wNbOfPoints = static_cast<int>(2 / wSampleRate);

    // Get maximum and minimum values of the data.
    double wMaxValue = maxValue(iSource, iIndex);
    double wMinValue = minValue(iSource, iIndex);

    // Get the average value of the data.
    double wAvg = averageValue(iSource, iIndex);

    // Get the base ratio that is used to define a peak.
    double wBaseRatio =  appsettings->value(nullptr, GC_NOTIO_BCVX_FIND_PEAK_BASE_RATIO, 0.05).toDouble();

    // Verify if a maximum and a minimum peak can't be found.
    if (((wAvg * (1 + wBaseRatio)) > wMaxValue) && ((wAvg * (1 - wBaseRatio)) < wMinValue))
    {
        // Define the average as the value half way between the maximum and the minimum.
        wAvg = wMaxValue - ((wMaxValue - wMinValue) / 2);

        // Divide the base ratio by 2.
        wBaseRatio /= 2;
    }

    // Calculate the maximum and minimum ratios.
    double wMaxAvgRatio = 1 + wBaseRatio, wMaxPeakRatio = 1 - wBaseRatio / 2;
    double wMinAvgRatio = 1 - wBaseRatio, wMinPeakRatio = 1 + wBaseRatio / 2;

#ifdef GC_DEBUG
    qDebug() << "Ratio Max Avg:" << wMaxAvgRatio << "Ratio Max Peak:" << wMaxPeakRatio;
    qDebug() << "Ratio Min Avg:" << wMinAvgRatio << "Ratio Min Peak:" << wMinPeakRatio;

    qDebug() << "Curve average:" << wAvg;
    qDebug() << "Curve max value:" << wMaxValue;
    qDebug() << "Curve min value:" << wMinValue;
#endif

    // Find peaks.
    for (int i = 0; i < iSource.count(); i += static_cast<int>(1 / wSampleRate))
    {
        // Check if the range used for peak analysis is out of bound.
        if (((i - wNbOfPoints) < 0) || ((i + wNbOfPoints) >= iSource.count()))
            continue;

        // Get metric value for the current index.
        double wCurrentValue = iSource.at(i)->number[iIndex];

        // Get metric value for the range lower bound index.
        double wLowerBoundValue = iSource.at(i - wNbOfPoints)->number[iIndex];

        // Get metric value for the range higher bound index.
        double wHigherBoundValue = iSource.at(i + wNbOfPoints)->number[iIndex];

        // Define a previous index twice the range.
        int wPreviousCheckPt = i - wNbOfPoints * 2;

        // Define a next index twice the range.
        int wNextCheckPt = i + wNbOfPoints * 2;

        // Current metric value is greater than a certain ratio of the average value.
        if (wCurrentValue > (wAvg * wMaxAvgRatio))
        {
            bool wFound = false;

            // Previous check point is valid.
            if (wPreviousCheckPt >= 0)
            {
                // Get the metric value at the previous validation point.
                double wValidatePeak = iSource.at(wPreviousCheckPt)->number[iIndex];

                // A the lower bound value is smaller than current value and the previous validation point
                // is smaller than a ratio of the current value.
                if ((wLowerBoundValue < wCurrentValue) && (wValidatePeak < (wCurrentValue * wMaxPeakRatio)))
                    wFound = true;
            }

            // Next check point is valid.
            if (wNextCheckPt < iSource.count())
            {
                // Get the metric value at the next validation point.
                double wValidatePeak = iSource.at(wNextCheckPt)->number[iIndex];

                // A the higher bound value is smaller than current value and the next validation point
                // is smaller than a ratio of the current value.
                if ((wHigherBoundValue < wCurrentValue) && (wValidatePeak < (wCurrentValue * wMaxPeakRatio)))
                    wFound = true;
            }

            // Peak found.
            if (wFound)
            {
                // Check if the peak overlap the last found peak to filter noise.
                if (!wMaxPeaks.isEmpty() && (wMaxPeaks.last().range[1] > (i - wNbOfPoints)))
                {
                    // Keep the last peak if its greater than the current one.
                    if (wMaxPeaks.last().y > (wCurrentValue * iFactor))
                        continue;
                    // The current peak is higher, so remove the last one.
                    else
                    {
                        wMaxPeaks.removeLast();
                    }
                }

                // Create a maximum peak.
                s_peak wTemp = { false, i, iSource.at(i)->secs, wCurrentValue * iFactor, { i - wNbOfPoints, i + wNbOfPoints }, 0 };

                // Calculate integral.
                for (int j = wTemp.range[0]; j < wTemp.range[1]; j++)
                {
                    wTemp.integralValue += wSampleRate * iFactor * (iSource.at(j)->number[iIndex]  + iSource.at(j + 1)->number[iIndex]) / 2;
                }
                wMaxPeaks.append(wTemp);
            }
        }
        // Current metric value is lower than a certain ratio of the average value.
        else if ((wCurrentValue < (wAvg * wMinAvgRatio)) && (wCurrentValue > 0.0))
        {
            bool wFound = false;

            // Previous check point is valid.
            if (wPreviousCheckPt >= 0)
            {
                // Get the metric value at the previous validation point.
                double wValidatePeak = iSource.at(wPreviousCheckPt)->number[iIndex];

                // A the lower bound value is higher than current value and the previous validation point
                // is greater than a ratio of the current value.
                if ((wLowerBoundValue > wCurrentValue) && (wValidatePeak > (wCurrentValue * wMinPeakRatio)))
                    wFound = true;
            }

            // Next check point is valid.
            if (wNextCheckPt < iSource.count())
            {
                // Get the metric value at the next validation point.
                double wValidatePeak = iSource.at(wNextCheckPt)->number[iIndex];

                // A the higher bound value is higher than current value and the next validation point
                // is greater than a ratio of the current value.
                if ((wHigherBoundValue > wCurrentValue) && (wValidatePeak > (wCurrentValue * wMinPeakRatio)))
                    wFound = true;
            }

            // Peak found.
            if (wFound)
            {
                // Check if the peak overlap the last found peak to filter noise.
                if (!wMinPeaks.isEmpty() && (wMinPeaks.last().range[1] > (i - wNbOfPoints)))
                {
                    // Keep the last peak if its smaller than the current one.
                    if (wMinPeaks.last().y < (wCurrentValue * iFactor))
                        continue;
                    // The current peak is smaller, so remove the last one.
                    else {
                        wMinPeaks.removeLast();
                    }
                }

                // Create a minimum peak.
                s_peak wTemp = { true, i, iSource.at(i)->secs, wCurrentValue * iFactor, { i - wNbOfPoints, i + wNbOfPoints }, 0 };

                // Calculate integral.
                for (int j = wTemp.range[0]; j < wTemp.range[1]; j++)
                {
                    wTemp.integralValue += wSampleRate * iFactor * (iSource.at(j)->number[iIndex]  + iSource.at(j + 1)->number[iIndex]) / 2;
                }
                wMinPeaks.append(wTemp);
            }
        }
    }

    // Merge peaks vectors.
    wReturning.clear();
    wReturning.append(wMinPeaks);
    wReturning.append(wMaxPeaks);

    // Sort chronologically.
    qSort(wReturning.begin(), wReturning.end(), CorrectBCVx::byTimeStamp);

#ifdef GC_DEBUG
    for (auto &wDebug : wReturning)
        qDebug() << "Peak" << (wDebug.isMin ? "min" : "max") << secsToString(wDebug.x) << "Metric Value:" << wDebug.y << "Integral value:" << wDebug.integralValue;

    qDebug() << "Number of peaks:" << wReturning.count();
#endif

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::matchPeaks
///        This method matches peaks from two XDataPoint sources.
///        It goes through Source 1 peaks vector trying to match with the
///        second source. When a prospect is found, it tries to find subsequent
///        peaks that are matching from that point in both sources.
///
/// \param[in] iSource1Peaks    Found peaks vector reference.
/// \param[in] iSource2Peaks    Found peaks vector reference.
/// \param[in] iIntervalLength  Duration of the interval.
///
/// \return A vector of matching peaks.
///////////////////////////////////////////////////////////////////////////////
QVector<QPair<s_peak, s_peak>> CorrectBCVx::matchPeaks(QVector<s_peak> &iSource1Peaks, QVector<s_peak> &iSource2Peaks, const double iIntervalLength)
{
    QVector<QPair<s_peak, s_peak>> wReturning;

    // Get matching peaks algorithm parameters used to debug.
    double yPeak = appsettings->value(nullptr, GC_NOTIO_BCVX_MATCH_PEAK_Y, 0.02).toDouble();
    double intgPeak = appsettings->value(nullptr, GC_NOTIO_BCVX_MATCH_PEAK_INTG, 0.025).toDouble();
    double ySubSq = appsettings->value(nullptr, GC_NOTIO_BCVX_MATCH_PEAK_SUBSQ_Y, 0.02).toDouble();
    double intgSubSq = appsettings->value(nullptr, GC_NOTIO_BCVX_MATCH_PEAK_SUBSQ_INTG, 0.03).toDouble();

    // Determine how many subsequent peaks should match to distinguish the good results.
    // Varying from 3 to 10 consecutive peaks depending on timespan.
    double wSubSqFactor = appsettings->value(nullptr, GC_NOTIO_BCVX_MATCH_PEAK_SUBSQ_FACTOR, 0.0015).toDouble();

    const int wNbSequential = std::min(10, static_cast<int>(wSubSqFactor * iIntervalLength) + 3);
    qDebug() << "Number of subsequent peaks to validate:" << wNbSequential;

    // Matching peaks algorithm.
    auto wPeakRef = iSource2Peaks.begin();
    for (auto &wPeak1Itr : iSource1Peaks)
    {
        // Find a source 2 peak that match the source 1 first peak.
        for (auto wPeak2Itr = wPeakRef; wPeak2Itr != iSource2Peaks.end(); wPeak2Itr++)
        {
            // Get the integral error between two peaks.
            double wCurrentMarginError = std::abs(((wPeak2Itr->integralValue - wPeak1Itr.integralValue) / wPeak1Itr.integralValue));

            // Get the peak value error between the two.
            double wCurrentErrorY = abs(wPeak2Itr->y -wPeak1Itr.y) / wPeak1Itr.y;

            // The errors between two peaks must be smaller than a specific value.
            if ((wCurrentErrorY < yPeak) && (wCurrentMarginError < intgPeak) && (wPeak1Itr.isMin == wPeak2Itr->isMin))
            {
                // First subsequent peak valid.
                int wValidPeaks = 1;
                bool wAddPeak = false;

                // Get the next peak from both sources.
                auto wNextPeak1Itr = (&wPeak1Itr + 1);
                auto wNextPeak2Itr = (wPeak2Itr + 1);

                // Next peak is out of bound.
                if (wNextPeak1Itr == iSource1Peaks.end() || wNextPeak2Itr == iSource2Peaks.end())
                {
                    if (wNextPeak1Itr == iSource1Peaks.end())
                        qDebug() << "Next Peak1 vector out of bound.";

                    if (wNextPeak2Itr == iSource2Peaks.end())
                        qDebug() << "Next Peak2 vector out of bound.";

                    // We consider that the peak is valid.
                    wAddPeak = true;
                    break;
                }

                // Try to find the N subsequent peaks.
                for (int i = 0; i < (wNbSequential - 1); i++)
                {
                    wNextPeak1Itr = (&wPeak1Itr + i);
                    wNextPeak2Itr = (wPeak2Itr + i);

                    // Get the integral error between two peaks.
                    double wNextMarginError = std::abs(((wNextPeak2Itr->integralValue - wNextPeak1Itr->integralValue) / wNextPeak1Itr->integralValue));

                    // Get the peak value error between the two.
                    double wNextErrorY = abs(wNextPeak2Itr->y - wNextPeak1Itr->y) / wNextPeak1Itr->y;

                    // The errors between two peaks must be smaller than a specific value.
                    if ((wNextErrorY < ySubSq) && (wNextMarginError < intgSubSq) && (wNextPeak1Itr->isMin == wNextPeak2Itr->isMin))
                    {
                        // One more subsequent peak.
                        wValidPeaks++;

                        // Check if we got the number of valid subsequent peaks.
                        wAddPeak = (wValidPeaks == wNbSequential);
                    }
                    else
                    {
                        // No subsequent mathing peaks. The peak is not valid.
                        wValidPeaks = 0;
                        wAddPeak = false;
                        break;
                    }
                }

                // Add a peak if enough subsequent matching peaks are found.
                if (wAddPeak)
                {
                    // Save a reference to the peak of the second source matching with Source 1.
                    // We don't want the peak to be added more than once.
                    wPeakRef = wPeak2Itr + 1;
                    wReturning.append(QPair<s_peak, s_peak>(wPeak1Itr, *wPeak2Itr));

                    break;
                }
            }
        }
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectBCVx::postProcess
///        This method starts the data processing.
///
/// \param[in/out]  ride    Current RideFile object.
/// \param[in/out]  config  Data processor configuration.
/// \param[in]      op      Operation type.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool CorrectBCVx::postProcess(RideFile *ride, DataProcessorConfig *config = nullptr, QString op = "") {
    int wManualOffset = -5;
    int wFlags = eFlagType::Power;
    bool wAuto = true;
    RideItem *rideItem = nullptr;

    // Error message window.
    QString wInformativeText;
    QString wMsgBoxText;
    QMessageBox wMsgBox(config);
    wMsgBox.setWindowTitle(tr("Correct BCVx using Garmin"));
    wMsgBox.setWindowModality(Qt::WindowModal);
    wMsgBox.setStandardButtons(QMessageBox::Abort);
    wMsgBox.setIcon(QMessageBox::Warning);

    if (ride && ride->context)
    {
        rideItem = ride->context->ride;

        if (rideItem == nullptr)
            return false;

        XDataSeries *wBCVxSeries = ride->xdata("BCVX");
        XDataSeries *wGarminSeries = ride->xdata("GARMIN");

        // Get defaults values from settings.
        if (config == nullptr){
            // Called automatically.
            wManualOffset = appsettings->value(nullptr, GC_NOTIO_BCVX_OFFSET, -5).toInt();
            wFlags = appsettings->value(nullptr, GC_NOTIO_BCVX_FLAGS, eFlagType::Power).toInt();
            wAuto = appsettings->value(nullptr, GC_NOTIO_BCVX_AUTO, true).toBool();
        }
        // Called manually.
        else {
            wManualOffset = static_cast<CorrectBCVxConfig *>(config)->offsetEdit->value();
            wFlags = static_cast<CorrectBCVxConfig *>(config)->flagsEdit->currentIndex() + 1;
            wAuto = static_cast<CorrectBCVxConfig *>(config)->m_autoOffset->isChecked();
        }

        // Initialize the offset with the user defined.
        int wOffsetResultValue = wManualOffset;

        // No BCVX section.
        if (wBCVxSeries == nullptr )
        {
            wInformativeText = tr("Notio raw data (BCVx) missing.");
        }
        // No Garmin section.
        else if ( wGarminSeries == nullptr )
        {
            wInformativeText = tr("No Garmin data available.");
        }
        // No intervals selected.
        else if (rideItem->intervalsSelected().empty())
        {
            wInformativeText = tr("No intervals selected.");
            wMsgBox.setIcon(QMessageBox::Information);
        }
        // Automatic detection is activated.
        else if (wAuto)
        {
            // Build a message to display to the user before proceeding.
            wMsgBoxText = tr("Auto offset setting is activated.");
            if (!rideItem->intervalsSelected(RideFileInterval::ALL).isEmpty())
            {
                wInformativeText = tr("Cannot fix for entire ride. Please inspect and select intervals.");
                wMsgBox.setIcon(QMessageBox::Information);
            }
            else
            {
                wMsgBoxText.append("\n\n" + tr("The manual offset will be taken into account if none can be determined automatically."));
                wInformativeText = tr("Are you sure you want to continue?");
                wMsgBox.setStandardButtons(QMessageBox::Abort | QMessageBox::Yes);
                wMsgBox.setButtonText(QMessageBox::Yes, tr("Continue"));
            }
        }

        // Display error message.
        if (!wInformativeText.isEmpty())
        {
            int wMsgBoxReturn = 0;
            // Display message only on manual execution.
            if ((op == "UPDATE") && (config != nullptr))
            {
                wMsgBox.setText(wMsgBoxText);
                wMsgBox.setInformativeText(wInformativeText);
                wMsgBoxReturn = wMsgBox.exec();
            }

            // User aborted the operation.
            if (wMsgBoxReturn == QMessageBox::Abort)
                return false;
        }

        appsettings->syncQSettingsGlobal();
        QMap<QString, int> wIntervalOffsetApplied;

        QString wDetailedText = tr("Time offset applied to selected intervals:") + "\n\n";
        QString wIntervalsNotFixedText;

        // Fix for all selected intervals.
        ride->command->startLUW("Fix BCVx with Garmin");
        foreach(IntervalItem *interval, rideItem->intervals()) {
            if (interval->selected) {
                cout << "Interval selected" << endl;
                int intervalStart = static_cast<int>(interval->start);
                int intervalStop = static_cast<int>(interval->stop);
                cout << "Interval start" << intervalStart << endl;
                cout << "Interval stop" << intervalStop << endl;

                int wResult = fixBCVx(ride, intervalStart, intervalStop, wOffsetResultValue, wAuto, static_cast<unsigned int>(wFlags));
                qDebug() << "Garmin offset" << wOffsetResultValue << endl;

                // Set details text.
                QString wResultDetailsText;

                // BCVx has been fixed.
                if (wResult != cErrorOffset)
                {
                    wResultDetailsText = QString::number(wOffsetResultValue) + " " + QString(tr("second")) + ((std::abs(wOffsetResultValue) <= 1) ? "" : "s");

                    // No offset has been found automatically.
                    if (wResult == CorrectBCVx::Manual)
                    {
                        // Indicate the user defined offset is used.
                        wResultDetailsText = tr("Manual offset");
                    }
                    wDetailedText.append(QString("%1\t\t%2\n").arg(interval->name, -20, ' ').arg(wResultDetailsText));
                }
                else
                {
                    wIntervalsNotFixedText.append(QString("%1\n").arg(interval->name));
                }

                // Reset offset to user defined for the next interval.
                wOffsetResultValue = wManualOffset;
            }
        }
        ride->command->endLUW();

        // Auto setting is on and the ride has been modified.
        if (wAuto && ride->command->undoCount())
        {
            wMsgBox.setText(tr("The selected intervals have been automatically fixed.\n\n"
                               "You will need to \"compute\" your activity if you accept changes."));
            wMsgBox.setInformativeText(tr("See details to validate."));
            wMsgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Reset);
            wMsgBox.setButtonText(QMessageBox::Ok, tr("Accept"));
            wMsgBox.setButtonText(QMessageBox::Reset, tr("Undo"));

            if (!wIntervalsNotFixedText.isEmpty())
            {
                wIntervalsNotFixedText.prepend("\n" + tr("The following intervals have nothing to fix:") + "\n\n");
            }
            wMsgBox.setDetailedText(wDetailedText + wIntervalsNotFixedText);

            // Show result dialog.
            int wTestReturn = wMsgBox.exec();

            // Undo button pressed.
            if (wTestReturn == QMessageBox::Reset)
            {
                qDebug() << "Undo fix";
                ride->command->undoCommand();
            }
        }
        return true;
    }
    return false;
}
