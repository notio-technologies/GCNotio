/*
 * CorrectDrops.cpp
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
#ifdef GC_HAVE_NOTIOALGO
#include "MiscAlgo.h"
#endif
#include "NotioData.h"

using namespace std;

// Config widget used by the Preferences/Options config panes
class CorrectPowerDrops;

///////////////////////////////////////////////////////////////////////////////
/// \brief The CorrectPowerDropsConfig class
///        This class defines the configuration widget used by the Preferences/
///        Options config panes for "Correct BCVx Power Drops" data processor.
///////////////////////////////////////////////////////////////////////////////
class CorrectPowerDropsConfig: public DataProcessorConfig {
    Q_DECLARE_TR_FUNCTIONS( CorrectPowerDropsConfig)

    friend class CorrectPowerDrops;
protected:

    QHBoxLayout *mlayout;

public:

    // Constructor
    CorrectPowerDropsConfig(QWidget *parent) :
        DataProcessorConfig(parent) {

        setContentsMargins(0, 0, 0, 0);

        HelpWhatsThis *help = new HelpWhatsThis(parent);
        parent->setWhatsThis(
                    help->getWhatsThisText(
                        HelpWhatsThis::MenuBar_Edit_FixNotioPowerDrops));

        mlayout = new QHBoxLayout(this);
        mlayout->setContentsMargins(0, 0, 0, 0);
    }

    //~CorrectPowerDropsConfig() {} // deliberately not declared since Qt will delete
    // the widget and its children when the config pane is deleted

    // Descriptive text within the configuration dialog window.
    QString explain() {
        return QString(tr("Fix power meter drops in BCVx. \n\n"
                          "Fill missing power values with the previous valid power. "
                          "Do it for each selected interval.\n\n"
                          "You will need to \"compute\" your activity after processing."));
    }

    // Read data processor parameters.
    void readConfig() {
        // No parameters.
    }

    // Save data processor parameters.
    void saveConfig() {
        // No parameters.
    }
};

// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
///////////////////////////////////////////////////////////////////////////////
/// \brief The CorrectPowerDrops class
///        This class defines a data processor used to correct power lost in
///        BCVx XData series.
///////////////////////////////////////////////////////////////////////////////
class CorrectPowerDrops: public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS( CorrectPowerDrops)

public:
    CorrectPowerDrops() {
    }
    ~CorrectPowerDrops() {
    }

    // The processor
    bool postProcess(RideFile *ride, DataProcessorConfig *config, QString op);

    // The config widget
    DataProcessorConfig* processorConfig(QWidget *parent) {
        return new CorrectPowerDropsConfig(parent);
    }

    // Localized Name
    QString name() {
        return (tr("Correct BCVx Power Drops"));
    }

private:
    int fixDrops(RideFile *ride, int iStart, int iEnd);
};

// Register data processor.
static bool CorrectPowerDropsAdded =
        DataProcessorFactory::instance().registerProcessor(
            QString("_Correct Power Drops"), new CorrectPowerDrops());

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectPowerDrops::fixDrops
///        This method is called by the processor and is used to fill the
///        missing power data in the BCVx XData series.
///
/// \param[in/out]  ride     Current RideFile object.
/// \param[in]      iStart   Interval start time.
/// \param[in]      iEnd     Interval stop time.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
int CorrectPowerDrops::fixDrops(RideFile *ride, int iStart, int iEnd)
{
    // Get BCVx XData series.
    XDataSeries *series = ride->xdata("BCVX");

    // Series not available.
    if ( series == nullptr )
        return -1;

    // BCVx data indexes.
    int statusByteIndex = series->valuename.indexOf("statusByte");
    int BCVxPowerIndex = series->valuename.indexOf("power");

    // If there is no status byte column or power column, no point to continue.
    if ((statusByteIndex < 0) || (BCVxPowerIndex < 0))
    {
        return -1;
    }

    // Determine sample rate.
    XDataPoint* xpoint1 = series->datapoints.at(0);
    XDataPoint* xpoint2 = series->datapoints.at(1);
    double BCVxSampleRate = (xpoint2->secs - xpoint1->secs);

    cout << "SampleRate " << BCVxSampleRate << endl;

    if ((BCVxSampleRate > 0) == false)
        BCVxSampleRate = 1.0;

    int startPoint = static_cast<int>(iStart / BCVxSampleRate);
    int endPoint = static_cast<int>(iEnd / BCVxSampleRate);

    // Fix power drops.
    double lastPower = 0;
    for (int i = startPoint; i < endPoint ; i++) {
        XDataPoint* xpoint = series->datapoints.at(i);
        double power = xpoint->number[BCVxPowerIndex];
        uint16_t statusByte = static_cast<uint16_t>(xpoint->number[statusByteIndex]);

        // Check if there is no power value and if the power meter connection status is disconnected.
        if (((power > 0.0) == false) && ((statusByte & NotioData::cPowerStatus) == 0)) {
            // Get the index of the next valid power value.
            int j;
            for (j=i+1; ((power > 0.0) == false) && ((statusByte & NotioData::cPowerStatus) == 0) && (j < endPoint) ; j++) {
                XDataPoint* xpoint = series->datapoints.at(j);
                power = xpoint->number[BCVxPowerIndex];
                statusByte = static_cast<uint16_t>(xpoint->number[statusByteIndex]);
            }

            // Fill missing values.
            if ( j < endPoint ) {
                cout << "Drop from " << i << " to " << j << " seconds " << (j-i)*BCVxSampleRate << endl;
                if ( ((j-i)*BCVxSampleRate) < 10.0 ) {
                    cout << "Correct " << endl;
                    for ( int k = i ; k < j ; k++ )
                        ride->command->setXDataPointValue("BCVX", k, BCVxPowerIndex + 2, lastPower );
                }
            }
            i = j;
        }
        // Last valid power value.
        else {
            lastPower = power;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectPowerDrops::postProcess
///        This method starts the data processing.
///
/// \param[in/out]  ride    Current RideFile object.
/// \param[in/out]  config  Data processor configuration.
/// \param[in]      op      Operation type.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool CorrectPowerDrops::postProcess(RideFile *ride, DataProcessorConfig *config = nullptr, QString op = "")
{
    RideItem *rideItem = nullptr;

    // Error message window.
    QString wErrorMsg;
    QMessageBox wMsgBox(config);
    wMsgBox.setWindowTitle(tr("Correct BCVx Power Drops"));
    wMsgBox.setWindowModality(Qt::WindowModality::WindowModal);
    wMsgBox.setStandardButtons(QMessageBox::Ok);
    wMsgBox.setIcon(QMessageBox::Warning);

    if (ride && ride->context)
    {
        rideItem = ride->context->ride;

        if (rideItem == nullptr)
            return false;

        // Cannot continue if no BCVX data.
        if ( ride->xdata("BCVX") == nullptr )
        {
            wErrorMsg = tr("Notio raw data (BCVx) missing.");
        }
        // Need at least one selected interval.
        else if (rideItem->intervalsSelected().empty())
        {
            wErrorMsg = tr("No intervals selected.");
            wMsgBox.setIcon(QMessageBox::Information);
        }

        // Show error message.
        if (!wErrorMsg.isEmpty())
        {
            // Display message only on manual execution.
            if ((op == "UPDATE") && (config != nullptr))
            {
                wMsgBox.setText(wErrorMsg);
                wMsgBox.exec();
            }
            return false;
        }

        ride->command->startLUW("Fix power drops");
        foreach(IntervalItem *interval, rideItem->intervals()) {
            if (interval->selected) {
                cout << "Interval selected" << endl;
                int intervalStart = static_cast<int>(interval->start);
                int intervalStop = static_cast<int>(interval->stop);
                cout << "Interval start" << intervalStart << endl;
                cout << "Interval stop" << intervalStop << endl;
                fixDrops(ride, intervalStart, intervalStop);
            }
        }
        ride->command->endLUW();
        return true;
    }
    return false;
}

// Config widget used by the Preferences/Options config panes
class CorrectSpeedDrops;

///////////////////////////////////////////////////////////////////////////////
/// \brief The CorrectSpeedDropsConfig class
///        This class defines the configuration widget used by the Preferences/
///        Options config panes for "Correct BCVx Speed Drops" data processor.
///////////////////////////////////////////////////////////////////////////////
class CorrectSpeedDropsConfig: public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(CorrectSpeedDropsConfig)

    friend class CorrectSpeedDrops;
protected:

    QHBoxLayout *wMainLayout;

public:

    // Constructor
    CorrectSpeedDropsConfig(QWidget *iParent) : DataProcessorConfig(iParent)
    {
        setContentsMargins(0, 0, 0, 0);

        HelpWhatsThis *wHelp = new HelpWhatsThis(iParent);
        iParent->setWhatsThis(
                    wHelp->getWhatsThisText(
                        HelpWhatsThis::MenuBar_Edit_FixNotioSpeedDrops));

        wMainLayout = new QHBoxLayout(this);
        wMainLayout->setContentsMargins(0, 0, 0, 0);
    }

    //~CorrectSpeedDropsConfig() {} // deliberately not declared since Qt will delete
    // the widget and its children when the config pane is deleted

    // Descriptive text within the configuration dialog window.
    QString explain()
    {
        return QString(tr("Fix speed sensor drops in BCVx. \n\n"
                          "Interpolate missing speed values for "
                          "drops up to 10 seconds. \n"
                          "Do it for each selected interval.\n\n"
                          "You will need to \"compute\" your activity after processing."));
    }

    // Read data processor parameters.
    void readConfig() {} // No parameters.

    // Save data processor parameters.
    void saveConfig() {} // No parameters.
};

// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
///////////////////////////////////////////////////////////////////////////////
/// \brief The CorrectSpeedDrops class
///        This class defines a data processor used to correct speed loss in
///        BCVx XData series.
///////////////////////////////////////////////////////////////////////////////
class CorrectSpeedDrops: public DataProcessor
{
    Q_DECLARE_TR_FUNCTIONS(CorrectSpeedDrops)

public:
    CorrectSpeedDrops() {}
    ~CorrectSpeedDrops() {}

    // The processor
    bool postProcess(RideFile *iRide, DataProcessorConfig *iConfig, QString iOp);

    // The config widget
    DataProcessorConfig* processorConfig(QWidget *iParent) { return new CorrectSpeedDropsConfig(iParent); }

    // Localized Name
    QString name() { return (tr("Correct BCVx Speed Drops")); }

private:
    static int const cDuplicatedSamplesMin = 4;
    static double constexpr cMaxDropDuration = 10.1;        // In seconds.

    int fixDrops(RideFile *iRide, int iStart, int iEnd);
};

// Register data processor.
static bool CorrectSpeedDropsAdded =
        DataProcessorFactory::instance().registerProcessor(QString("_Correct Speed Drops"), new CorrectSpeedDrops());

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectSpeedDrops::fixDrops
///        This method is called by the processor and is used to fill the
///        missing speed data in the BCVx XData series.
///
/// \param[in/out]  iRide     Current RideFile object.
/// \param[in]      iStart   Interval start time.
/// \param[in]      iEnd     Interval stop time.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
int CorrectSpeedDrops::fixDrops(RideFile *iRide, int iStart, int iEnd)
{
    // Get BCVx XData series.
    XDataSeries *wDataSeries = iRide->xdata("BCVX");

    // Series not available.
    if ( wDataSeries == nullptr )
        return -1;

    // BCVx data indexes.
    int wStatusByteIndex = wDataSeries->valuename.indexOf("statusByte");
    int wBCVxSpeedIndex = wDataSeries->valuename.indexOf("speed");

    // If there is no status byte column or speed column, no point to continue.
    if ((wStatusByteIndex < 0) || (wBCVxSpeedIndex < 0))
        return -1;

    // Determine sample rate.
    XDataPoint* wPoint1 = wDataSeries->datapoints.at(0);
    XDataPoint* wPoint2 = wDataSeries->datapoints.at(1);
    double wBCVxSampleRate = (wPoint2->secs - wPoint1->secs);

    if ((wBCVxSampleRate > 0) == false)
        wBCVxSampleRate = 1.0;

    int wStartPoint = static_cast<int>(iStart / wBCVxSampleRate);
    int wEndPoint = static_cast<int>(iEnd / wBCVxSampleRate);

    // Fix speed drops.
    double wPrevSpeed = 0;
    int wSpeedIsInvalid = -1;
    int wConstSpeedCntNb = 0;
    for (int i = wStartPoint; i < wEndPoint; i++)
    {
        // Get the speed value and the speed status bit.
        XDataPoint* wCurrentPoint = wDataSeries->datapoints.at(i);
        double wSpeed = wCurrentPoint->number[wBCVxSpeedIndex];
        bool wSpeedStatusBit = static_cast<uint16_t>(wCurrentPoint->number[wStatusByteIndex]) & (NotioData::cSpeedStatus | NotioData::cSpeedCadStatus);

        // Check if there is no speed value.
        if ((wSpeed > 0.0) == false)
        {
            // Verify if the speed sensor connection status is disconnected.
            if (wSpeedStatusBit == false)
            {
                // Get the index of the next valid speed value.
                int j = i + 1;
                for (; j < wEndPoint; j++)
                {
                    XDataPoint* wNextValidPoint = wDataSeries->datapoints.at(j);
                    wSpeed = wNextValidPoint->number[wBCVxSpeedIndex];
                    wSpeedStatusBit = static_cast<uint16_t>(wNextValidPoint->number[wStatusByteIndex]) & (NotioData::cSpeedStatus | NotioData::cSpeedCadStatus);

                    // We found a valid speed.
                    if ((wSpeed > 0.0) && wSpeedStatusBit)
                        break;
                }

                // How much time we have invalid data. (no speed and invalid status)
                double wTimeElapsed = (j - i) * wBCVxSampleRate;

                // Check if we have repetitive speed value or a null speed before the detection of the drop.
                if (wSpeedIsInvalid > 0)
                {
                    // Change the index of the begining of the drop detected.
                    i = wSpeedIsInvalid;

                    // Reset invalid speed index and speed duplication counter for the next detection.
                    wSpeedIsInvalid = -1;
                    wConstSpeedCntNb = 0;
                }

                // Get speed before the drop at the buffer start.
                double wLastValidSpeed = 0.0;
				if (i > 0)
                    wLastValidSpeed = wDataSeries->datapoints.at(i - 1)->number[wBCVxSpeedIndex];

                // Get speed after the drop at the buffer end.
                wSpeed = wDataSeries->datapoints.at(j)->number[wBCVxSpeedIndex];

                // Fill missing values when the speed loss is less than 10 seconds.
                if (wTimeElapsed < cMaxDropDuration)
                {
                    // Interpolate speed.
                    for (int k = 0; k < (j - i); k++)
                    {
#ifdef GC_HAVE_NOTIOALGO
                        double wNewSpeed = GcAlgo::MiscAlgo::interpolateSpeedSample(k, j - i, wLastValidSpeed, wSpeed);
#else
                        double wNewSpeed = wLastValidSpeed + (k + 1) * (wSpeed - wLastValidSpeed) / (j - i);
#endif
                        iRide->command->setXDataPointValue("BCVX", i + k, wBCVxSpeedIndex + 2, wNewSpeed);
                    }
                }
                // Get ready for the next detection.
                i = j;
            }
            // Speed has reached 0.
            else if (wSpeedIsInvalid == -1)
            {
                wSpeedIsInvalid = i;
            }
        }
        // There is more than 4 samples with the same value before drop detection.
        else if ((wConstSpeedCntNb > cDuplicatedSamplesMin) && (wSpeedIsInvalid == -1))
        {
            wSpeedIsInvalid = i - cDuplicatedSamplesMin;
        }
        // Speed is the same has the previous value, increment counter.
        else if ((fabs(wPrevSpeed - wSpeed) > 0.0) == false)
        {
            wConstSpeedCntNb++;
        }
        // Speed seems good.
        else
        {
            // Reset invalid speed index and speed duplication counter for the next detection.
            wConstSpeedCntNb = 0;
            wSpeedIsInvalid = -1;
        }

        wPrevSpeed = wSpeed;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CorrectSpeedDrops::postProcess
///        This method starts the data processing.
///
/// \param[in/out]  iRide    Current RideFile object.
/// \param[in/out]  iConfig  Data processor configuration.
/// \param[in]      iOp      Operation type.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool CorrectSpeedDrops::postProcess(RideFile *iRide, DataProcessorConfig *iConfig = nullptr, QString iOp = "")
{
    RideItem *wRideItem = nullptr;

    // Error message window.
    QString wErrorMsg;
    QMessageBox wMsgBox(iConfig);
    wMsgBox.setWindowTitle(tr("Correct BCVx Speed Drops"));
    wMsgBox.setWindowModality(Qt::WindowModality::WindowModal);
    wMsgBox.setStandardButtons(QMessageBox::Ok);
    wMsgBox.setIcon(QMessageBox::Warning);

    if (iRide && iRide->context)
    {
        wRideItem = iRide->context->ride;

        if (wRideItem == nullptr)
            return false;

        // Cannot continue if no BCVX data.
        if ( iRide->xdata("BCVX") == nullptr )
        {
            wErrorMsg = tr("Notio raw data (BCVx) missing.");
        }
        // Need at least one selected interval.
        else if (wRideItem->intervalsSelected().empty())
        {
            wErrorMsg = tr("No intervals selected.");
            wMsgBox.setIcon(QMessageBox::Information);
        }

        // Show error message.
        if (!wErrorMsg.isEmpty())
        {
            // Display message only on manual execution.
            if ((iOp == "UPDATE") && (iConfig != nullptr))
            {
                wMsgBox.setText(wErrorMsg);
                wMsgBox.exec();
            }
            return false;
        }

        iRide->command->startLUW("Fix speed drops");
        foreach(IntervalItem *wIntervalItr, wRideItem->intervals()) {
            if (wIntervalItr->selected) {
                cout << "Interval selected" << endl;
                int wIntervalStart = static_cast<int>(wIntervalItr->start);
                int wIntervalStop = static_cast<int>(wIntervalItr->stop);
                cout << "Interval start" << wIntervalStart << endl;
                cout << "Interval stop" << wIntervalStop << endl;
                fixDrops(iRide, wIntervalStart, wIntervalStop);
            }
        }
        iRide->command->endLUW();
        return true;
    }
    return false;
}
