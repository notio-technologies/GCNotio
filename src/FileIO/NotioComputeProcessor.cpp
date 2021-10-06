#include "NotioComputeProcessor.h"

#include "pre_process.h"
#include "slope_estimation.h"
#include "MiscAlgo.h"
#include "NotioComputeFunctions.h"
#include "GcUpgrade.h"
#include "RideItem.h"
#include "RideFileCommand.h"
#include "RideFile.h"
#include "MainWindow.h"
#include <QGroupBox>

using namespace NotioComputeFunctions;
using namespace NotioData;
using namespace GcAlgo;

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessorConfig::NotioComputeProcessorConfig
///        Constructor.
///
/// \param[in] iParent Parent widget.
///////////////////////////////////////////////////////////////////////////////
NotioComputeProcessorConfig::NotioComputeProcessorConfig(QWidget *iParent) :
    DataProcessorConfig(iParent)
{
    HelpWhatsThis *help = new HelpWhatsThis(iParent);
    iParent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_NotioComputeData));

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setContentsMargins(0, 0, 0, 0);

    // Basic parameters.
    m_weightLabel = new QLabel(tr("Total Weight:"));
    m_weightLabel->setToolTip(tr("Total rider weight including bike and accessories."));
    m_weightDoubleSpinBox = new QDoubleSpinBox();
    m_weightDoubleSpinBox->setRange(35, 150);
    m_weightDoubleSpinBox->setSingleStep(0.1);
    m_weightDoubleSpinBox->setSuffix(QString(" ") + tr("kg"));

    QHBoxLayout *wTotalWeightLayout = new QHBoxLayout();
    wTotalWeightLayout->addWidget(m_weightLabel);
    wTotalWeightLayout->addWidget(m_weightDoubleSpinBox);

    m_wheelLabel = new QLabel(tr("Tire Size:"));
    m_wheelLabel->setToolTip(tr("Wheel circumference."));
    m_wheelSpinBox = new QSpinBox();
    m_wheelSpinBox->setRange(100, 5000);
    m_wheelSpinBox->setSingleStep(10);
    m_wheelSpinBox->setSuffix(QString(" ") + tr("mm"));

    QHBoxLayout *wTireLayout = new QHBoxLayout();
    wTireLayout->addWidget(m_wheelLabel);
    wTireLayout->addWidget(m_wheelSpinBox);

    m_crrLabel = new QLabel(tr("Crr:"));
    m_crrLabel->setToolTip(tr("Rolling Resistance Coefficient."));
    m_crrDoubleSpinBox = new QDoubleSpinBox();
    m_crrDoubleSpinBox->setRange(0.001, 0.015);
    m_crrDoubleSpinBox->setDecimals(5);
    m_crrDoubleSpinBox->setSingleStep(0.0001);

    QHBoxLayout *wCrrLayout = new QHBoxLayout();
    wCrrLayout->addWidget(m_crrLabel);
    wCrrLayout->addWidget(m_crrDoubleSpinBox);

    m_riderFactorLabel = new QLabel(tr("Calibration Factor:"));
    m_riderFactorLabel->setToolTip(tr("Notio device calibration factor."));
    m_riderFactorDoubleSpinBox = new QDoubleSpinBox();
    m_riderFactorDoubleSpinBox->setRange(1, 2);
    m_riderFactorDoubleSpinBox->setDecimals(4);
    m_riderFactorDoubleSpinBox->setSingleStep(0.001);

    QHBoxLayout *wRiderFactorLayout = new QHBoxLayout();
    wRiderFactorLayout->addWidget(m_riderFactorLabel);
    wRiderFactorLayout->addWidget(m_riderFactorDoubleSpinBox);

    // Advanced parameters.
    m_altLabel = new QLabel(tr("Altitude type:"));
    m_altLabel->setToolTip(tr("Altitude correction type."));
    m_altComboBox = new QComboBox();
    m_altComboBox->addItem(tr("Corrected"));
    m_altComboBox->addItem(tr("Constant"));

    QHBoxLayout *wAltLayout = new QHBoxLayout();
    wAltLayout->addWidget(m_altLabel);
    wAltLayout->addWidget(m_altComboBox);

    m_mecEffLabel = new QLabel(tr("Mechanical Efficiency:"));
    m_mecEffLabel->setToolTip(tr("Power efficiency of mechanical parts."));
    m_mecEffDoubleSpinBox = new QDoubleSpinBox();
    m_mecEffDoubleSpinBox->setRange(0.8, 1.2);
    m_mecEffDoubleSpinBox->setSingleStep(0.05);

    QHBoxLayout *wMecEffLayout = new QHBoxLayout();
    wMecEffLayout->addWidget(m_mecEffLabel);
    wMecEffLayout->addWidget(m_mecEffDoubleSpinBox);

    m_speedOffsetLabel = new QLabel(tr("Speed Sensor's Delay:"));
    m_speedOffsetLabel->setToolTip(tr("Manual time offset between Notio and the speed sensor."));
    m_speedOffsetDoubleSpinBox = new QDoubleSpinBox();
    m_speedOffsetDoubleSpinBox->setRange(0, 5);
    m_speedOffsetDoubleSpinBox->setSingleStep(0.25);
    m_speedOffsetDoubleSpinBox->setSuffix(QString(" ") + tr("seconds"));

    QHBoxLayout *wSpeedLayout = new QHBoxLayout();
    wSpeedLayout->addWidget(m_speedOffsetLabel);
    wSpeedLayout->addWidget(m_speedOffsetDoubleSpinBox);

    // Development parameters.
    m_riderExponentLabel = new QLabel(tr("Exponent:"));
    m_riderExponentLabel->setToolTip(tr("Notio calibration exponent."));
    m_riderExponentDoubleSpinBox = new QDoubleSpinBox();
    m_riderExponentDoubleSpinBox->setRange(-1.0, 1.0);
    m_riderExponentDoubleSpinBox->setDecimals(3);
    m_riderExponentDoubleSpinBox->setSingleStep(0.05);

    QHBoxLayout *wRiderExponentLayout = new QHBoxLayout();
    wRiderExponentLayout->addWidget(m_riderExponentLabel);
    wRiderExponentLayout->addWidget(m_riderExponentDoubleSpinBox);

    m_inertiaFactorLabel = new QLabel(tr("Inertia Factor:"));
    m_inertiaFactorLabel->setToolTip(tr("Notio device inertia factor."));
    m_inertiaFactorDoubleSpinBox = new QDoubleSpinBox();
    m_inertiaFactorDoubleSpinBox->setRange(0.8, 1.2);
    m_inertiaFactorDoubleSpinBox->setDecimals(4);
    m_inertiaFactorDoubleSpinBox->setSingleStep(0.001);

    QHBoxLayout *wInertiaLayout = new QHBoxLayout();
    wInertiaLayout->addWidget(m_inertiaFactorLabel);
    wInertiaLayout->addWidget(m_inertiaFactorDoubleSpinBox);

    QString wGroupBoxStyle = QString::fromUtf8("QGroupBox { border:1px solid gray; border-radius:3px; margin-top: 1ex; }"
                                               "QGroupBox::title { subcontrol-origin: margin; subcontrol-position:top left; left: 20px; padding: 0 5px; }");

    // Manual Dialog window.
    if (iParent->windowType() == Qt::Dialog)
    {
        QVBoxLayout *wC1Layout = new QVBoxLayout();

        wC1Layout->addStretch(1);
        wC1Layout->addLayout(wTotalWeightLayout);
        wC1Layout->addLayout(wCrrLayout);

        QVBoxLayout *wC2Layout = new QVBoxLayout();

        wC2Layout->addStretch(1);
        wC2Layout->addLayout(wTireLayout);
        wC2Layout->addLayout(wRiderFactorLayout);

        QVBoxLayout *wC3Layout = new QVBoxLayout();

        wC3Layout->addStretch(0);
        wC3Layout->addLayout(wAltLayout);
        wC3Layout->addLayout(wMecEffLayout);
        wC3Layout->addLayout(wSpeedLayout);

        QVBoxLayout *wC4Layout = new QVBoxLayout();

        wC4Layout->addStretch(1);
        wC4Layout->addLayout(wRiderExponentLayout);
        wC4Layout->addLayout(wInertiaLayout);

        QGroupBox *wParametersGroup = new QGroupBox(tr("Basic"));
        wParametersGroup->setStyleSheet(wGroupBoxStyle);
        QHBoxLayout *wParamLayout = new QHBoxLayout();
        wParamLayout->addLayout(wC1Layout);
        wParamLayout->addLayout(wC2Layout);

        wParametersGroup->setLayout(wParamLayout);
        m_layout->addWidget(wParametersGroup);

        QGroupBox *wAdvParamGroup = new QGroupBox(tr("Advanced"));
        wAdvParamGroup->setStyleSheet(wGroupBoxStyle);
        QHBoxLayout *wAdvLayout = new QHBoxLayout();
        wAdvLayout->addLayout(wC3Layout);

        wAdvParamGroup->setLayout(wAdvLayout);
        m_layout->addWidget(wAdvParamGroup);

        // Display development parameters.
        if (MainWindow::gc_devMode)
        {
            QGroupBox *wDevGroup = new QGroupBox(tr("Development"));
            wDevGroup->setStyleSheet(wGroupBoxStyle);
            QHBoxLayout *wDevLayout = new QHBoxLayout();
            wDevLayout->addLayout(wC4Layout);

            wDevGroup->setLayout(wDevLayout);
            m_layout->addWidget(wDevGroup);
        }
    }
    // Options page.
    else
    {
        m_layout->addLayout(wAltLayout);
        m_layout->addLayout(wTotalWeightLayout);
        m_layout->addLayout(wRiderFactorLayout);
        m_layout->addLayout(wTireLayout);
        m_layout->addLayout(wCrrLayout);
        m_layout->addLayout(wMecEffLayout);
        m_layout->addLayout(wSpeedLayout);

        // Display development parameters.
        if (MainWindow::gc_devMode)
        {
            m_layout->addLayout(wRiderExponentLayout);
            m_layout->addLayout(wInertiaLayout);
        }

        m_layout->addStretch();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessorConfig::explain
///        This method generates a descriptive text to show the user in the
///        processor manual configuration dialog window.
///
/// \return A text explaining the data processor functionality.
///////////////////////////////////////////////////////////////////////////////
QString NotioComputeProcessorConfig::explain()
{
    QString wDescription = QString(tr("Compute Notio data to be able to analyze your CdA for a "
                                      "selected ride. Set the parameters to configure the process.\n"
                                      " - Enter your total weight during the activity.\n"
                                      " - Set the rolling resistance, keep the default value if you don't know.\n"
                                      " - Write the calibration factor from your iOS app or the one that you previously found in GC.\n"
                                      " - Don't forget to set the tire size of your bike. Ex: 700 x 28c = 2136mm\n\n"
                                      "Advanced settings\n"
                                      " - Altitude type: Selects the altitude correction type. A constant altitude "
                                      "should be used for velodrome for example.\n"
                                      " - Mechanical efficiency (%1p): Allows to take into account watts mechanically lost.\n"
                                      " - Speed Sensor's delay: Offsets speed data to compensate for delay between the "
                                      "sensor and the Notio device.\n\n"
                                      )).arg(QChar(0xB7, 0x03));

    // Display development parameters.
    if (MainWindow::gc_devMode)
        wDescription.append(tr("Development mode\n"
                               " - Exponent: Exponent used to calibrate Notio Pitot tube.\n"
                               " - Inertia Factor: Coefficient to take into account watts lost/gained by the wheel inertia."));
    return wDescription;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::postProcess
///        This method computes the Notio data for the user to be able to
///        analyze his ride and find his CdA.
///
/// \param[in] iRide    Ride file to process.
/// \param[in] iConfig  Data processor configuration.
/// \param[in] iOp      Operation type.
///
/// \return The process final status.
///////////////////////////////////////////////////////////////////////////////
bool NotioComputeProcessor::postProcess(RideFile *iRide, DataProcessorConfig *iConfig, QString iOp)
{
    bool wReturning = false;
    int wCurrentStep = eSteps::INIT;
    int wTotalSteps = eSteps::NB_STEPS;
    qDebug() << "Compute operation:" << iOp;

    // Check if the user ask manually to compute the data either by the edit menu or the Notio tools or CdA Analysis chart.
    bool wLaunchedEditMenu = ((iOp == "UPDATE") && iConfig);
    bool wDisplayDialog = (iOp == "COMPUTE") || (iOp == "ALTITUDE") || (iOp == "SPEED");

    // Error message window.
    QString wErrorMsg;
    QMessageBox wMsgBox(iConfig);
    wMsgBox.setWindowTitle(tr("Compute Notio Data"));
    wMsgBox.setWindowModality(Qt::WindowModality::WindowModal);
    wMsgBox.setStandardButtons(QMessageBox::Ok);
    wMsgBox.setIcon(QMessageBox::Warning);

    // Ride exist and it is not set to deletion.
    // Skip if the operation is ADD (when importing and auto process is set to Save)
    if (iRide && (iOp != "DELETE") && (iOp != "ADD"))
    {
        // Old Notio file have "ROW" XDataSeries instead of "BCVx".
        XDataSeries *wBcvxSeries = iRide->xdata("BCVX");
        if (wBcvxSeries == nullptr)
            convertRow2Bcvx(iRide);

        // Before computing the data, check for speed and power sensors data availability.
        // A power meter and a speed sensor are necessary to calculate CdA.
        if (NotioFuncCompute::speedPowerUnavailable(iRide, wErrorMsg) == false)
        {
            iRide->command->startLUW(tr("Compute Notio Data"));
            iRide->context->notifyCustomProgressStart(wDisplayDialog);
            QString wStatusMessage = tr("Computing ride data.");

            do
            {
                // Display progression.
                iRide->context->notifyCustomProgressUpdate(static_cast<int>(wCurrentStep * 100 / wTotalSteps));
                iRide->context->notifyProgressDialogMessage(wStatusMessage);
                QApplication::processEvents();

                switch (wCurrentStep)
                {
                case eSteps::INIT:
                {
                    iRide->setRecIntSecs(iRide->getTag("sampleRate", "0.25").toDouble());

                    // Load computation parameters.
                    loadConfiguration(iRide, iConfig);

                    // Remove RideData series.
                    if (iRide->xdata("RideData"))
                        iRide->command->removeXData("RideData");

                    // Remove standard data.
                    iRide->command->deletePoints(0, iRide->dataPoints().count());

                    // Reset data present status flags.
                    for (int i = 0; i < RideFile::SeriesType::none; i++)
                    {
                        iRide->command->setDataPresent(static_cast<RideFile::SeriesType>(i), false);
                    }

                    // Remove CDAData series.
                    if (iRide->xdata("CDAData"))
                        iRide->command->removeXData("CDAData");
                }
                    break;
                case eSteps::LOAD_RIDEDATA:
                    // Create RideData.
                    if (loadNotioData(iRide) == false)
                        wErrorMsg = tr("Cannot create RideData");
                    break;
                case eSteps::LOAD_CDADATA:
                    // Compute CdA data used to display graphs.
                    loadDisplayData(iRide);
                    iRide->command->endLUW();

                    // Next step: save ride.
                    wStatusMessage = tr("Saving ride.");
                    break;
                case eSteps::SAVE:
                    if (wLaunchedEditMenu || wDisplayDialog)
                    {
                        iRide->context->mainWindow->saveSilent(iRide->context, iRide->context->ride);
                    }
                    // Next step: refresh ride.
                    wStatusMessage = tr("Refreshing data.");
                    break;
                case eSteps::REFRESH:
                    iRide->context->notifyRideChanged(iRide->context->ride);
                    break;
                default:
                    break;
                }

                wCurrentStep++;
            } while ((wCurrentStep < wTotalSteps) && wErrorMsg.isEmpty());

            if (wErrorMsg.isEmpty())
                wReturning = true;
            else
            {
                iRide->command->endLUW();
                iRide->command->undoCommand();
            }
        }
        QApplication::processEvents();
        iRide->context->notifyCustomProgressEnd();
    }

    // Show error message.
    if (!wErrorMsg.isEmpty())
    {
        // Display message only on manual execution.
        if (wLaunchedEditMenu || wDisplayDialog)
        {
            wMsgBox.setText(wErrorMsg);
            wMsgBox.exec();
        }
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::convertRow2Bcvx
///        This method converts the ROW series to BCVX series. In an old version,
///        the XData series for Notio data was named ROW. To support those ride,
///        we copy the old series, if exists, create the BCVx series and delete
///        the old one. We could only rename the series but if we want to be
///        able to undo change, we do this using commands.
///
/// \param[in] iRide    Ride file.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool NotioComputeProcessor::convertRow2Bcvx(RideFile *iRide)
{
    bool wReturning = false;
    if (iRide)
    {
        XDataSeries *wRowSeries = iRide->xdata("ROW");
        if (wRowSeries)
        {
            iRide->command->startLUW(tr("Convert ROW series"));
            XDataSeries *wBcvxSeries = new XDataSeries(*wRowSeries);
            wBcvxSeries->name = "BCVX";
            iRide->command->addXData(wBcvxSeries);
            iRide->command->removeXData("ROW");
            iRide->command->endLUW();
            wReturning = true;
        }
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::loadConfiguration
///        This method will load the data processor configuration.
///        If no configuration is given meaning it is an automatic process,
///        Ride metadata will be used during process.
///
///        This method also migrates old metadata fields used in Notio
///        computation to new fields implemented by the cloud service.
///
/// \param[in] iRide    Ride file.
/// \param[in] iConfig  Manual configuration.
///////////////////////////////////////////////////////////////////////////////
void NotioComputeProcessor::loadConfiguration(RideFile *iRide, DataProcessorConfig *iConfig)
{
    qDebug() << Q_FUNC_INFO;

    NotioComputeProcessorConfig *wConfig = static_cast<NotioComputeProcessorConfig *>(iConfig);

    // Get global defaults values from settings.
    QString wTireSize = appsettings->value(nullptr, GC_NOTIO_DATA_WHEEL_SIZE, 2136).toString();
    QString wWeight = appsettings->value(nullptr, GC_NOTIO_DATA_TOTAL_WEIGHT, 80).toString();
    QString wRiderFactor = appsettings->value(nullptr, GC_NOTIO_DATA_RIDER_FACTOR, 1.39).toString();
    QString wCrr = appsettings->value(nullptr, GC_NOTIO_DATA_CRR, 0.004).toString();
    QString wMecEff = appsettings->value(nullptr, GC_NOTIO_DATA_MECH_EFF, 1.0).toString();
    QString wAltType = appsettings->value(nullptr, GC_NOTIO_DATA_ALT_TYPE, 0).toString();

    // Development parameters
    QString wRiderExponent = appsettings->value(nullptr, GC_NOTIO_DATA_RIDER_EXPONENT, -0.05).toString();
    QString wInertiaFactor = appsettings->value(nullptr, GC_NOTIO_DATA_INERTIA_FACTOR, 1.15).toString();

    // Migrate old compute parameters to new ones.
    if (iRide->getTag("customCrr", "").isEmpty())
        iRide->setTag("customCrr", iRide->getTag("notio.crr", iRide->getTag("crr", wCrr)));

    if (iRide->getTag("customRiderFactor", "").isEmpty())
        iRide->setTag("customRiderFactor", iRide->getTag("notio.riderFactor", iRide->getTag("riderFactor", wRiderFactor)));

    if (iRide->getTag("customWeight", "").isEmpty())
        iRide->setTag("customWeight", iRide->getTag("notio.riderWeight", iRide->getTag("weight", wWeight)));

    if (iRide->getTag("customTireSize", "").isEmpty())
        iRide->setTag("customTireSize", iRide->getTag("notio.newWheelCirc", iRide->getTag("wheelCirc", wTireSize)));

    if (iRide->getTag("customMecEff", "").isEmpty())
        iRide->setTag("customMecEff", iRide->getTag("notio.efficiency", wMecEff));

    // Development parameters.
    if (iRide->getTag("customInertia", "").isEmpty())
        iRide->setTag("customInertia", iRide->getTag("notio.inertiaFactor", wInertiaFactor));

    if (iRide->getTag("customExponent", "").isEmpty())
        iRide->setTag("customExponent", iRide->getTag("notio.riderExponent", iRide->getTag("stickFactor", wRiderExponent)));

    // Remove old parameters. To be uncommented in future release.
//    iRide->removeTag("notio.crr");
//    iRide->removeTag("notio.riderFactor");
//    iRide->removeTag("notio.riderWeight");
//    iRide->removeTag("notio.newWheelCirc");
//    iRide->removeTag("notio.efficiency");
//    iRide->removeTag("notio.inertiaFactor");
//    iRide->removeTag("notio.riderExponent");

    // For compatibility issues, rides computed with this version will be compatible with previous GC version.
    // To be removed in future release.
    iRide->setTag("notio.crr", iRide->getTag("customCrr", iRide->getTag("crr", wCrr)));
    iRide->setTag("notio.riderWeight", iRide->getTag("customWeight", iRide->getTag("weight", wWeight)));
    iRide->setTag("notio.efficiency", iRide->getTag("customMecEff", wMecEff));
    iRide->setTag("notio.inertiaFactor", iRide->getTag("customInertia", wInertiaFactor));
    iRide->setTag("notio.riderFactor", iRide->getTag("customRiderFactor", iRide->getTag("riderFactor", wRiderFactor)));
    iRide->setTag("notio.riderExponent", iRide->getTag("customExponent", iRide->getTag("stickFactor", wRiderExponent)));
    iRide->setTag("notio.newWheelCirc", iRide->getTag("customTireSize", iRide->getTag("wheelCirc", wTireSize)));
    // ==================================

    // Set data processor manual configuration.
    if (wConfig)
    {
        iRide->setTag("customTireSize", QString::number(wConfig->m_wheelSpinBox->value()));
        iRide->setTag("customWeight", QString::number(wConfig->m_weightDoubleSpinBox->value()));
        iRide->setTag("customRiderFactor", QString::number(wConfig->m_riderFactorDoubleSpinBox->value()));
        iRide->setTag("customCrr", QString::number(wConfig->m_crrDoubleSpinBox->value()));
        iRide->setTag("customMecEff", QString::number(wConfig->m_mecEffDoubleSpinBox->value()));
        iRide->setTag("notio.altMethod", wConfig->m_altComboBox->currentIndex() == 0 ? "altEKF" : "altZero");
        iRide->setTag("notio.speedSensorDelay", QString::number(wConfig->m_speedOffsetDoubleSpinBox->value()));

        // Development parameters
        iRide->setTag("customExponent", QString::number(wConfig->m_riderExponentDoubleSpinBox->value()));
        iRide->setTag("customInertia", QString::number(wConfig->m_inertiaFactorDoubleSpinBox->value()));

        // For compatibility issues, rides computed with this version will be compatible with previous GC version.
        // To be removed in future release.
        iRide->setTag("notio.crr", QString::number(wConfig->m_crrDoubleSpinBox->value()));
        iRide->setTag("notio.riderWeight", QString::number(wConfig->m_weightDoubleSpinBox->value()));
        iRide->setTag("notio.efficiency", QString::number(wConfig->m_mecEffDoubleSpinBox->value()));
        iRide->setTag("notio.inertiaFactor", QString::number(wConfig->m_inertiaFactorDoubleSpinBox->value()));
        iRide->setTag("notio.riderFactor", QString::number(wConfig->m_riderFactorDoubleSpinBox->value()));
        iRide->setTag("notio.riderExponent", QString::number(wConfig->m_riderExponentDoubleSpinBox->value()));
        iRide->setTag("notio.newWheelCirc", QString::number(wConfig->m_wheelSpinBox->value()));
        // ==================================
    }

    // Add GC Notio build version in JSON.
    iRide->setTag("GC Version", QString::number(NK_VERSION_LATEST));
    iRide->setTag("GC Min Version", QString::number(NK_MIN_VERSION));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::loadNotioData
///        This method loads data from BCVx series into a intermediary
///        XDataSeries RideData and into the standard data.
///
/// \param[in] iRide    Ride file.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool NotioComputeProcessor::loadNotioData(RideFile *iRide)
{
    // Get the original wheel circumference.
    QString wOriginalWheelCirc = iRide->getTag("wheelCirc", "2136");
    double wWheelCirc = wOriginalWheelCirc.toDouble();

    // Get the corrected wheel circumference.
    double wNewWheelCirc = iRide->getTag("customTireSize", wOriginalWheelCirc).toDouble();

    // Calculate a correction factor to apply to speed data.
    double wSpeedCorrectionFactor = (wWheelCirc > 0) && (wNewWheelCirc > 0) ? (wNewWheelCirc / wWheelCirc) : 1.0;

    // Get calibration factor and rider exponent.
    double wRiderFactor = iRide->getTag("customRiderFactor", iRide->getTag("notio.riderFactor", "1.39")).toDouble();
    double wRiderExponent = iRide->getTag("customExponent", iRide->getTag("notio.riderExponent", "-0.05")).toDouble();

    XDataSeries *wBcvxSeries = iRide->xdata("BCVX");
    if (wBcvxSeries == nullptr)
    {
        qDebug() << "no BCVX data";
        return false;
    }

    // Get BCVx data series columns' index.
    int wSpeedIndex = wBcvxSeries->valuename.indexOf("speed");
    int wPowerIndex = wBcvxSeries->valuename.indexOf("power");
    int wHrmIndex = wBcvxSeries->valuename.indexOf("hrm");
    int wCadenceIndex = wBcvxSeries->valuename.indexOf("cadence");
    int wHumidityIndex = wBcvxSeries->valuename.indexOf("humidity");
    int wTemperatureIndex = wBcvxSeries->valuename.indexOf("temperature");
    int wAltitudeIndex = wBcvxSeries->valuename.indexOf("altitude");
    int wAltitudeCorrIndex = wBcvxSeries->valuename.indexOf("altitudeCorr");
    int wAccXIndex = wBcvxSeries->valuename.indexOf("accelerometerX");
    int wAccYIndex = wBcvxSeries->valuename.indexOf("accelerometerY");
    int wAccZIndex = wBcvxSeries->valuename.indexOf("accelerometerZ");
    int wGyrXIndex = wBcvxSeries->valuename.indexOf("gyroscopeX");
    int wGyrYIndex = wBcvxSeries->valuename.indexOf("gyroscopeY");
    int wGyrZIndex = wBcvxSeries->valuename.indexOf("gyroscopeZ");
    int wBaroPressureIndex = wBcvxSeries->valuename.indexOf("pressure");
    int wAirPressureIndex = wBcvxSeries->valuename.indexOf("airSpeed");
    int wGpsLonIndex = wBcvxSeries->valuename.indexOf("gpsLongitude");
    int wGpsLatIndex = wBcvxSeries->valuename.indexOf("gpsLatitude");
    int wStatusByteIndex = wBcvxSeries->valuename.indexOf("statusByte");
    int wFrontGearIndex = wBcvxSeries->valuename.indexOf("frontGear");
    int wRearGearIndex = wBcvxSeries->valuename.indexOf("rearGear");

    // Create RideData series.
    XDataSeries *wRideDataSeries = iRide->xdata("RideData");
    if (wRideDataSeries == nullptr)
    {
        wRideDataSeries = new XDataSeries();
        wRideDataSeries->name = "RideData";
        wRideDataSeries->valuename = rideDataValuename;
        wRideDataSeries->unitname = rideDataUnitname;

        iRide->command->addXData(wRideDataSeries);
    }
    else
        iRide->command->deleteXDataPoints("RideData", 0, wRideDataSeries->datapoints.count());

    int numPoints = wBcvxSeries->datapoints.count();
    QVector<struct RideFilePoint> newRows;
    QVector<XDataPoint *> wRideDataRows;

    // Get BCVx series sample rate.
    double wBcvxSampleRate = NotioFuncCompute::estimateRecInterval(wBcvxSeries);

    // Get speed sensor data delay. (Speed sensor can have a certain amount of delay when sending data)
    const int wNbSamplesOffset = static_cast<int>(iRide->getTag("notio.speedSensorDelay", "0").toDouble() / wBcvxSampleRate);

    // Initialize heading calculation variables.
    double wHeading = 0.0, wPrevLat = 0.0, wPrevLon = 0.0;

    // Get Garmin first altitude sample value.
    int wGarminAltIndex = -1;
    XDataSeries *wGarminSeries = iRide->xdata("GARMIN");
    double wGarminbaseAltitude = 0.0;
    if (wGarminSeries != nullptr)
    {
        wGarminAltIndex = wGarminSeries->valuename.indexOf("gpsAltitude");
        if (wGarminAltIndex > -1)
        {
            XDataPoint* xpoint = wGarminSeries->datapoints.at(0);
            double wInitialAltitude = (wAltitudeIndex > -1) ? wBcvxSeries->datapoints[0]->number[wAltitudeIndex] : 0;
            wGarminbaseAltitude = xpoint->number[wGarminAltIndex] - wInitialAltitude;
        }
    }

    // Calculate distance.
    QVector<double> wCalculatedDistance = preCalculateDist(wBcvxSeries, wBcvxSampleRate, wNbSamplesOffset, wNewWheelCirc, wSpeedCorrectionFactor);

    // Check if distance has been calculated correctly.
    if (wCalculatedDistance.count() != numPoints)
        return false;

    for (int i = 0; i < numPoints; i++)
    {
        XDataPoint* wBcvxPoint = wBcvxSeries->datapoints.at(i);

        // Get xdata point at a position determined by user defined speed sensor delay.
        XDataPoint *xPointSpeedOffset = nullptr;
        if (i < (wBcvxSeries->datapoints.count() - wNbSamplesOffset))
            xPointSpeedOffset = wBcvxSeries->datapoints.at(i + wNbSamplesOffset);

        // Get speed.
        double wSpeed = 0.0;
        if (xPointSpeedOffset)
        {
            wSpeed = xPointSpeedOffset->number[wSpeedIndex] / 3.6 * wSpeedCorrectionFactor;
        }

        // Initialize air data variables.
        double wTemperature = 10000.0, wBaroPressure = 10000.0, wHumidity = 50.0;

        if (wBaroPressureIndex > -1)
            wBaroPressure = wBcvxPoint->number[wBaroPressureIndex];

        if (wHumidityIndex > -1)
            wHumidity = wBcvxPoint->number[wHumidityIndex];

        if (wTemperatureIndex > -1)
            wTemperature = wBcvxPoint->number[wTemperatureIndex];

        // Calculate air data.
        double wAirDensity = NotioFuncCompute::calculateAirDensity(wTemperature, wHumidity, wBaroPressure);
        double wAirPressure = wBcvxPoint->number[wAirPressureIndex] / AeroAlgo::cAirPressureSensorFactor;
        double wHeadwind = AeroAlgo::calculateHeadwind(wRiderFactor, wRiderExponent, wAirPressure, wAirDensity);

        // Get corrected altitude.
        double wAltCorrected = 0;
        if (wAltitudeCorrIndex > -1)
            wAltCorrected = wBcvxPoint->number[wAltitudeCorrIndex];
        else if (wAltitudeIndex > -1)
            wAltCorrected = wBcvxPoint->number[wAltitudeIndex];

        double wAltCompute = NotioFuncCompute::cAltConstant;

        if (iRide->getTag("notio.altMethod" ,"altEKF") == "altEKF")
            wAltCompute = wAltCorrected;

        // Adjust Garmin altitude.
        double wAltGarmin = 0.0;
        if (wGarminSeries)
        {
            // Get the time index in the Garmin data series.
            int wTimeIndex = wGarminSeries->timeIndex(wBcvxPoint->secs);

            // Calculate altitude
            if ((wTimeIndex > -1) && (wTimeIndex < wGarminSeries->datapoints.count()))
            {
                wAltGarmin = wGarminSeries->datapoints[wTimeIndex]->number[wGarminAltIndex] - wGarminbaseAltitude;
            }
        }

        // Calculate heading angle.
        if ((wGpsLatIndex > -1) && (wGpsLonIndex > -1))
        {
            double wLat = wBcvxPoint->number[wGpsLatIndex];
            double wLon = wBcvxPoint->number[wGpsLonIndex];

            if (i == 0)
            {
                wPrevLat = wLat;
                wPrevLon = wLon;
            }
            else if ((fabs(wLat) > .0001) && (fabs(wLon) > .0001) && (fabs(wLat - wPrevLat) > .0000001) && (fabs(wLon - wPrevLon) > .0000001))
            {
                wHeading = MiscAlgo::calculateHeading(wPrevLat, wPrevLon, wLat, wLon);
                wPrevLat = wLat;
                wPrevLon = wLon;
            }
        }

        // Set status byte.
        uint16_t wSensorsStatus = 0;
        if (wStatusByteIndex > -1)
        {
            // Speed sensor status bit.
            uint16_t statusByte  = static_cast<uint16_t>(wBcvxPoint->number[wStatusByteIndex]);
            bool wSpeedStatus = (xPointSpeedOffset) ? (static_cast<uint16_t>(xPointSpeedOffset->number[wStatusByteIndex]) & cSpeedStatus) : false;
            if (statusByte & cPowerStatus)
                wSensorsStatus |= 0x0004;

            if (wSpeedStatus)
                wSensorsStatus |= 0x0002;

            if (statusByte & cHrmStatus)
                wSensorsStatus |= 0x0001;
        }

        // Add data points.
        XDataPoint *wRideDataPoint = new XDataPoint();
        wRideDataPoint->secs = wBcvxPoint->secs;
        wRideDataPoint->km = wCalculatedDistance[i] / 1000;

        wRideDataPoint->number[rideDataIdx::ePower] = wBcvxPoint->number[wPowerIndex];
        wRideDataPoint->number[rideDataIdx::eSpeed] = wSpeed;
        wRideDataPoint->number[rideDataIdx::eAirPressure] = wBcvxPoint->number[wAirPressureIndex];
        wRideDataPoint->number[rideDataIdx::eAlt] = wBcvxPoint->number[wAltitudeIndex];
        wRideDataPoint->number[rideDataIdx::eAirDensity] = wAirDensity;
        wRideDataPoint->number[rideDataIdx::eAccX] = wBcvxPoint->number[wAccXIndex];
        wRideDataPoint->number[rideDataIdx::eAccY] = wBcvxPoint->number[wAccYIndex];
        wRideDataPoint->number[rideDataIdx::eAccZ] = wBcvxPoint->number[wAccZIndex];
        wRideDataPoint->number[rideDataIdx::eGyrX] = wBcvxPoint->number[wGyrXIndex];
        wRideDataPoint->number[rideDataIdx::eGyrY] = wBcvxPoint->number[wGyrYIndex];
        wRideDataPoint->number[rideDataIdx::eGyrZ] = wBcvxPoint->number[wGyrZIndex];
        wRideDataPoint->number[rideDataIdx::eAltCorrect] = wAltCorrected;
        wRideDataPoint->number[rideDataIdx::eAltGarmin] = wAltGarmin;
        wRideDataPoint->number[rideDataIdx::eAltCompute] = wAltCompute;
        wRideDataPoint->number[rideDataIdx::eHeading] = wHeading;
        wRideDataPoint->number[rideDataIdx::eLatitude] = wBcvxPoint->number[wGpsLatIndex];
        wRideDataPoint->number[rideDataIdx::eLongitude] = wBcvxPoint->number[wGpsLonIndex];
        wRideDataPoint->number[rideDataIdx::eTemperature] = wBcvxPoint->number[wTemperatureIndex];
        wRideDataPoint->number[rideDataIdx::eHeartrate] = wBcvxPoint->number[wHrmIndex];
        wRideDataPoint->number[rideDataIdx::eCadence] = wBcvxPoint->number[wCadenceIndex];
        wRideDataPoint->number[rideDataIdx::eFrontGear] = wBcvxPoint->number[wFrontGearIndex];
        wRideDataPoint->number[rideDataIdx::eRearGear] = wBcvxPoint->number[wRearGearIndex];
        wRideDataPoint->number[rideDataIdx::eStatus] = static_cast<double>(wSensorsStatus);

        wRideDataRows.append(wRideDataPoint);

        newRows << RideFilePoint(wBcvxPoint->secs, wBcvxPoint->number[wCadenceIndex], wBcvxPoint->number[wHrmIndex], wCalculatedDistance[i] / 1000,
                                 wSpeed * 3.6, 0, wBcvxPoint->number[wPowerIndex],  wAltCorrected, wBcvxPoint->number[wGpsLonIndex],
                                 wBcvxPoint->number[wGpsLatIndex], wHeadwind, 0, wBcvxPoint->number[wTemperatureIndex], 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        // Update data present status flags.
        for (int i = 0; i < RideFile::SeriesType::none; i++)
        {
            RideFile::SeriesType wSeriesType = static_cast<RideFile::SeriesType>(i);
            if (std::abs(newRows.last().value(wSeriesType)) > 0.0)
                iRide->command->setDataPresent(wSeriesType, true);
        }
    }

    iRide->command->appendXDataPoints("RideData", wRideDataRows);
    iRide->command->appendPoints(newRows);

    // Smooth the speed for acceleration from stationary position and deceleration to a stop.
    smoothSpeedCurve(iRide);

    // Will restore GPS data if missing from standard data.
    if (iRide->xdata("GARMIN"))
        restoreGpsData(iRide);

    // Get data from Dfly if missing from Notio data but is available from Garmin data.
    if (iRide->xdata("GEARS") && (iRide->getTag("dflyID", "0").toInt() == 0))
        getMissingDflyData(iRide);

    // Post process altitude.
    if ((iRide->getTag("notio.altMethod" ,"altEKF") == "altEKF"))
        adjustAlt(iRide);

    // Recalculate derived data.
    iRide->recalculateDerivedSeries(true);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::preCalculateDist
///        This methods calculates distance based on the revolution count of
///        the speed sensor and also based on the speed. It checks for
///        discrepencies between both total distance and returns the more
///        accurate distance vector for the ride.
///
/// \param[in] iXdataSeries     XData series which contains original data.
/// \param[in] iSampleRate      Sample rate of the XData series.
/// \param[in] iSampleOffset    Time offset between speed sensor and Notio.
/// \param[in] iTireSize        Wheel circumference.
/// \param[in] iSpeedCorrection Speed correction factor if tire size changed.
///
/// \return The more accurate distance vector for the ride.
///////////////////////////////////////////////////////////////////////////////
QVector<double> NotioComputeProcessor::preCalculateDist(XDataSeries *iXdataSeries, const double &iSampleRate, const int &iSampleOffset, const int &iTireSize, const double &iSpeedCorrection)
{
    // Calculated distance vectors.
    QVector<double> wDistFromRevCount, wDistFromSpeed;

    // No XDataSeries.
    if (iXdataSeries == nullptr)
        return QVector<double>();

    int numPoints = iXdataSeries->datapoints.count();

    // Get index of the speed and revCount series.
    int wSpeedIndex = iXdataSeries->valuename.indexOf("speed");
    int wRevCountIndex = iXdataSeries->valuename.indexOf("revCount");

    // Cannot find data.
    if ((wSpeedIndex < 0) || (wRevCountIndex < 0))
        return QVector<double>();

    // Initialize variables used for distance calculation.
    sRevCountAlgoVar wRevCountVar = { 0, 0, 0, false };

    for (int i = 0; i < numPoints; i++)
    {
        // Get xdata point at a position determined by user defined speed sensor delay.
        XDataPoint *xPointSpeedOffset = nullptr;
        if (i < (iXdataSeries->datapoints.count() - iSampleOffset))
            xPointSpeedOffset = iXdataSeries->datapoints.at(i + iSampleOffset);

        // Get speed and revolution count.
        double wSpeed = 0.0, wRevCount = 0.0;
        if (xPointSpeedOffset)
        {
            wSpeed = xPointSpeedOffset->number[wSpeedIndex] / 3.6 * iSpeedCorrection;
            wRevCount = xPointSpeedOffset->number[wRevCountIndex];
        }

        // Calculate distance in meters.
        wDistFromRevCount.append(NotioFuncCompute::calculateDist(wRevCount, iTireSize, wRevCountVar));
        wDistFromSpeed.append(wDistFromSpeed.count() ? (wSpeed * iSampleRate) + wDistFromSpeed.last() : 0.0);
    }

    // Return distance vector.
    double wErrorMargin = (std::abs(wDistFromSpeed.last() - wDistFromRevCount.last()) / (wDistFromSpeed.last() + wDistFromRevCount.last())) * 100;
    if (wErrorMargin > 0.5)
        return wDistFromSpeed;
    else
        return wDistFromRevCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::loadDisplayData
///        This method computes data used to be displayed in graph.
///
/// \param[in] iRide    Ride file.
///////////////////////////////////////////////////////////////////////////////
void NotioComputeProcessor::loadDisplayData(RideFile *iRide)
{
    XDataSeries *wRideDataSeries = iRide->xdata("RideData");
    if (wRideDataSeries == nullptr)
        return;

    int wRideDataCount = wRideDataSeries->datapoints.count();

    // Create CDAData series.
    XDataSeries *wCdaSeries  = iRide->xdata("CDAData");
    if (wCdaSeries == nullptr)
    {
        wCdaSeries = new XDataSeries();
        wCdaSeries->name = "CDAData";
        wCdaSeries->valuename = cdaDataValuename;
        wCdaSeries->unitname = cdaDataUnitname;

        iRide->command->addXData(wCdaSeries);
    }
    else
        iRide->command->deleteXDataPoints("CDAData", 0, wCdaSeries->datapoints.count());

    // Get RideData series columns' index.
    int wAirPressureIndex = wRideDataSeries->valuename.indexOf("airpressure");
    int wAirDensityIndex = wRideDataSeries->valuename.indexOf("airdensity");
    int wGyrX = wRideDataSeries->valuename.indexOf("gyrX");
    int wGyrY = wRideDataSeries->valuename.indexOf("gyrY");
    int wGyrZ = wRideDataSeries->valuename.indexOf("gyrZ");
    int wPowerIndex = wRideDataSeries->valuename.indexOf("power");
    int wSpeedIndex = wRideDataSeries->valuename.indexOf("speed");
    int wAltIndex = wRideDataSeries->valuename.indexOf("alt");
    int wAltGarminIndex = wRideDataSeries->valuename.indexOf("altGarmin");
    int wAltComputeIndex = wRideDataSeries->valuename.indexOf("altCompute");
    int wFrontGearIndex = wRideDataSeries->valuename.indexOf("frontGear");
    int wRearGearIndex = wRideDataSeries->valuename.indexOf("rearGear");

    // Get ride specific parameters.
    double wTotalWeight = iRide->getTag("customWeight", iRide->getTag("weight", "80")).toDouble();
    double wCrr = iRide->getTag("customCrr", iRide->getTag("crr", "0.0040")).toDouble();
    double wMechEff = iRide->getTag("customMecEff", "1.0").toDouble();
    double wInertiaFactor = iRide->getTag("customInertia", "1.15").toDouble();
    double wRiderFactor = iRide->getTag("customRiderFactor", iRide->getTag("notio.riderFactor", "1.39")).toDouble();
    double wRiderExponent = iRide->getTag("customExponent", iRide->getTag("notio.riderExponent", "-0.05")).toDouble();

    XDataPoint* wFirstPoint = wRideDataSeries->datapoints[0];
    double wPrevSpeed = wFirstPoint->number[wSpeedIndex];
    double wPrevAlt = wFirstPoint->number[wAltIndex];
    double wPrevAltCorrect = wFirstPoint->number[wAltComputeIndex];
    AeroAlgo::sEnergiesVectors wEnergyVectors;

    // Computes data in 1 Hz.
    int wNbSamples = static_cast<int>(1 / iRide->recIntSecs());

    // Initialize data for the first sample.
    wEnergyVectors.m_pmEnergy.push_back(0);
    wEnergyVectors.m_crrEnergy.push_back(0);
    wEnergyVectors.m_aeroEnergy.push_back(0);
    wEnergyVectors.m_kineticEnergy.push_back(0);
    wEnergyVectors.m_potentialEnergy.push_back(0);

    QVector<XDataPoint *> wRows;
    XDataPoint *p = new XDataPoint();
    p->secs = 0;
    p->km = 0.0;
    p->number[0] = 0;
    p->number[cdaDataIdx::eSpeedLegacy] = wPrevSpeed;
    p->number[cdaDataIdx::eBcvAlt] = wPrevAlt;
    p->number[cdaDataIdx::eEkfAlt] = wPrevAltCorrect;
    p->number[cdaDataIdx::eHuAlt] = wFirstPoint->number[wAltGarminIndex];

    p->number[cdaDataIdx::eETot] = wFirstPoint->number[wPowerIndex];
    p->number[cdaDataIdx::eECrr] = wFirstPoint->number[wSpeedIndex];
    p->number[cdaDataIdx::eEAlt] = wFirstPoint->number[wAltIndex];
    p->number[cdaDataIdx::eEDeH] = wPrevAltCorrect;
    p->number[cdaDataIdx::eEAcc] = 0.0;
    p->number[cdaDataIdx::eEAir] = AeroAlgo::pre_calc_aero_energy(wFirstPoint->number[wSpeedIndex], wFirstPoint->number[wAirPressureIndex] / AeroAlgo::cAirPressureSensorFactor);
    p->number[cdaDataIdx::eEBNO] = 0.0;

    p->number[cdaDataIdx::eWind] = 0.0;
    if (wFirstPoint->number[wAirPressureIndex] > 0.0)
    {
        double wAirPressure = wFirstPoint->number[wAirPressureIndex] / AeroAlgo::cAirPressureSensorFactor;
        double wHeadwind = AeroAlgo::calculateHeadwind(wRiderFactor, wRiderExponent, wAirPressure, wFirstPoint->number[wAirDensityIndex]) / 3.6;

        p->number[cdaDataIdx::eWind] = wHeadwind - wFirstPoint->number[wSpeedIndex];
    }

    p->number[cdaDataIdx::eAirdensity] = wFirstPoint->number[wAirDensityIndex];
    p->number[cdaDataIdx::eAirpressure] = wFirstPoint->number[wAirPressureIndex];

    p->number[cdaDataIdx::eRoll] = abs(wFirstPoint->number[wGyrX]) *.25;
    p->number[cdaDataIdx::eRollCRR] = MiscAlgo::calculateRollCrr(abs(p->number[cdaDataIdx::eRoll]), wTotalWeight, wFirstPoint->number[wSpeedIndex]);
    p->number[cdaDataIdx::eVibration] = 0.0;
    p->number[cdaDataIdx::eVibrationCRR] =  0.0;
    p->number[cdaDataIdx::eLostCdA] =  0.0;

    wRows.append(p);

    // Parse through data and calculate display data.
    for (int i = 0; i < (wRideDataSeries->datapoints.count() / wNbSamples); i++)
    {
        double wDistance = 0;
        double wPmEnergy = 0;
        double wSpeed = 0;
        double wPreCalcCrrEnergy = 0;
        double wAlt = 0;
        double wAltCorrect = 0;
        double wAltGarmin = 0;

        // Get delta altitude.
        double wDeltaAlt = 0;

        // Precalculate potential energy.
        double wPreCalcPotentialEnergy = 0;

        // Precalculate kinetic energy.
        double wPreCalcKineticEnergy = 0;

        // Get air pressure from pitot and calculated air density.
        double wAirPressure = 0;
        double wRho = 0;

        // Calculate wind.
        double wCalcWind = 0;

        // Precalculate CdA related energy.
        double wCdaEnergy = 0;

        // Find axes angles base on the gyroscopes for a sample.
        double wRoll = 0;
        std::vector<double> wVibrationBuffer;
        double wTurn = 0;

        // Get gears and find watts lost by gears.
        double wFrontGearValue = 0;
        double wRearGearValue = 0;
        double wChainPwrLoss = 0;

        // Calculate for one second.
        for (int j = 0 ; (j < wNbSamples) && (((wNbSamples * i) + j + 1) < wRideDataCount) ; j++)
        {
            int index = (wNbSamples * i) + j + 1;
            XDataPoint* xpoint = wRideDataSeries->datapoints.at(index);

            wDistance = xpoint->km;

            wSpeed = xpoint->number[wSpeedIndex];
            wPreCalcKineticEnergy += AeroAlgo::pre_calc_kinetic_energy(wSpeed, wPrevSpeed);
            wPrevSpeed = wSpeed;
            wPreCalcCrrEnergy += AeroAlgo::pre_calc_rolling_resistance_energy(wSpeed);

            wAlt = xpoint->number[wAltIndex];
            wDeltaAlt += wAlt - wPrevAlt;
            wPrevAlt = wAlt;

            wAltCorrect = xpoint->number[wAltComputeIndex];
            wPreCalcPotentialEnergy += AeroAlgo::pre_calc_potential_energy(wAltCorrect, wPrevAltCorrect);
            wPrevAltCorrect = wAltCorrect;

            wAltGarmin = xpoint->number[wAltGarminIndex];

            wPmEnergy += xpoint->number[wPowerIndex];

            wRoll += abs(xpoint->number[wGyrX]) * .25;
            wTurn += xpoint->number[wGyrZ] * .25;
            wVibrationBuffer.push_back(xpoint->number[wGyrY] * .25);

            double wMeasuredPressure = std::max(xpoint->number[wAirPressureIndex], 0.0) / AeroAlgo::cAirPressureSensorFactor;
            wCdaEnergy += AeroAlgo::pre_calc_aero_energy(wSpeed, wMeasuredPressure);
            wAirPressure += wMeasuredPressure;
            wRho += xpoint->number[wAirDensityIndex];

            wFrontGearValue = xpoint->number[wFrontGearIndex];
            wRearGearValue = xpoint->number[wRearGearIndex];
        }

        // Calculate vibration.
        double wVibe = MiscAlgo::calculateVibration(wVibrationBuffer);

        if (abs(wTurn) > 20.0)
            wVibe = 0.0;

        wPmEnergy /= wNbSamples;
        wPreCalcCrrEnergy /= wNbSamples;
        wCdaEnergy /= wNbSamples;
        wAirPressure /= wNbSamples;
        wRho /= wNbSamples;

        // Calculate wind.
        if (wAirPressure > 0.0)
        {
            double wHeadwind = AeroAlgo::calculateHeadwind(wRiderFactor, wRiderExponent, wAirPressure, wRho) / 3.6;
            wCalcWind = wHeadwind - wSpeed;
        }

        // Calculate energies for the current sample.
        wEnergyVectors.m_pmEnergy.push_back(wPmEnergy * wMechEff);
        wEnergyVectors.m_crrEnergy.push_back(AeroAlgo::rolling_resistance_energy(wCrr, wTotalWeight, wPreCalcCrrEnergy));
        wEnergyVectors.m_aeroEnergy.push_back(AeroAlgo::aero_energy(wRiderFactor, wRiderExponent, wAirPressure, wCdaEnergy));
        wEnergyVectors.m_kineticEnergy.push_back(AeroAlgo::kinetic_energy(wInertiaFactor, wTotalWeight, wPreCalcKineticEnergy));
        wEnergyVectors.m_potentialEnergy.push_back(AeroAlgo::potential_energy(wTotalWeight, wPreCalcPotentialEnergy));

        wChainPwrLoss = NotioFuncCompute::findGearPowerLost(wFrontGearIndex, wRearGearIndex);

        // Fill CDAData series.
        XDataPoint *p = new XDataPoint();
        p->secs = i + 1;
        p->km = wDistance * 1000;
        p->number[cdaDataIdx::eBcvAlt] = wAlt;
        p->number[cdaDataIdx::eEkfAlt] = wAltCorrect;
        p->number[cdaDataIdx::eHuAlt] = wAltGarmin;
        p->number[cdaDataIdx::eETot] = wPmEnergy;
        p->number[cdaDataIdx::eECrr] = wPreCalcCrrEnergy;
        p->number[cdaDataIdx::eEAlt] = wDeltaAlt;
        p->number[cdaDataIdx::eEDeH] = wPreCalcPotentialEnergy;
        p->number[cdaDataIdx::eEAcc] = wPreCalcKineticEnergy;
        p->number[cdaDataIdx::eEAir] = wCdaEnergy;
        p->number[cdaDataIdx::eWind] = wCalcWind;
        p->number[cdaDataIdx::eRoll] = wRoll;
        p->number[cdaDataIdx::eVibration] = wVibe * 10;
        p->number[cdaDataIdx::eXChain] = wChainPwrLoss;
        p->number[cdaDataIdx::eRawCDA] = 0.0;
        p->number[cdaDataIdx::eEkfCDA] = 0.0;

        // The following are to be removed in a future release.
        p->number[cdaDataIdx::eSpeedLegacy] = wSpeed;
        p->number[cdaDataIdx::eAirdensity] = wRho;
        p->number[cdaDataIdx::eAirpressure] = wAirPressure * AeroAlgo::cAirPressureSensorFactor;
        p->number[cdaDataIdx::eEBNO] = 0.0;
        p->number[cdaDataIdx::eRollCRR] = MiscAlgo::calculateRollCrr(wRoll, wTotalWeight, wSpeed);
        p->number[cdaDataIdx::eVibrationCRR] = MiscAlgo::calculateVibrationCrr(wVibe, wTotalWeight, wSpeed);
        p->number[cdaDataIdx::eFrontGearLegacy] = wFrontGearValue;
        p->number[cdaDataIdx::eRearGearLegacy] = wRearGearValue;
        p->number[cdaDataIdx::eAccCDA] = 0.0;
        p->number[cdaDataIdx::eWinCDA] = 0.0;
        p->number[cdaDataIdx::eLostCdA] = 0.0;
        p->number[cdaDataIdx::eWattsLost] = static_cast<double>(i % 10);

        wRows.append(p);
    }

    iRide->command->appendXDataPoints("CDAData", wRows);

    // Compute CdA to be displayed in the graphs with the calculated energies.
    computeCda2Display(iRide, wEnergyVectors);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::computeCda2Display
///        This method computes the CdA data to be displayed in graphs.
///
/// \param[in] iRide        Ride file.
/// \param[in] iEnergies    Energies involved in a ride.
///////////////////////////////////////////////////////////////////////////////
void NotioComputeProcessor::computeCda2Display(RideFile *iRide, GcAlgo::AeroAlgo::sEnergiesVectors &iEnergies)
{
    if (iRide && iRide->xdata("CDAData"))
    {
        XDataSeries *wCdaSeries = iRide->xdata("CDAData");

        int wRawCdaIndex = wCdaSeries->valuename.indexOf("rawCDA");
        int wEkfCdaIndex = wCdaSeries->valuename.indexOf("ekfCDA");

        int dataPoints = wCdaSeries->datapoints.count();
        double wPrevEkfCda = 0;

        int wTimeWindow = static_cast<int>(iRide->getTag("notio.cdaWindow","60").toInt() / NotioFuncCompute::estimateRecInterval(wCdaSeries) / 2);

        for (int i = 0; i < dataPoints; i++)
        {
            // Calcute time window boundaries around a sample.
            int wStartIndex = std::max(0, i - wTimeWindow);
            int wStopIndex = std::min(i + wTimeWindow, dataPoints - 1);

            // Calculates CdA with corrected altitude model.
            double wCda = AeroAlgo::calculate_cda(iEnergies, wStartIndex, wStopIndex);

            // Apply Low pass filter
            double wEkfCda = wCda;
            if (i > 0)
                wEkfCda = AeroAlgo::cda_low_pass_filter(wCda, wPrevEkfCda);

            wPrevEkfCda = wEkfCda;

            iRide->command->setXDataPointValue("CDAData", i, wRawCdaIndex + 2, wCda);
            iRide->command->setXDataPointValue("CDAData", i, wEkfCdaIndex + 2, wEkfCda);
        }

        // Compute values for window.
        if (iRide->getTag("notio.calcWindow", "60.00").toDouble() < 1) {
            iRide->setTag("notio.calcWindow", "1");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::smoothSpeedCurve
///        This method smoothes the speed data for acceleration from stationary
///        position and deceleration to a stop.
///
/// \param[in] iRide    Ride file.
///////////////////////////////////////////////////////////////////////////////
void NotioComputeProcessor::smoothSpeedCurve(RideFile *iRide)
{
    if (iRide)
    {
        XDataSeries *wRideDataSeries = iRide->xdata("RideData");

        // Smooth speed curve for acceleration from stationary position and deceleration to a stop.
        if (wRideDataSeries)
        {
            const double cAirPressureThreshold = 30.0;

            // Get data indexes.
            int wStatusByteIndex = wRideDataSeries->valuename.indexOf("status");
            int wSpeedIndex = wRideDataSeries->valuename.indexOf("speed");
            int wAirPressureIndex = wRideDataSeries->valuename.indexOf("airpressure");

            // Get RideData series sample rate.
            double wSampleRate = NotioFuncCompute::estimateRecInterval(wRideDataSeries);

            // Proceed if necessary data is found.
            if ((wSpeedIndex > 0) && (wStatusByteIndex > 0) && (wAirPressureIndex > 0))
            {
                int wConstantSpeedCount = 0;
                int wValidAirPressureCount = 0;
                double wPreviousSpeed = 0;

                // Scan the entire ride.
                for (int i = 0; i < wRideDataSeries->datapoints.count(); i++)
                {
                    double wCurrentSpeed = wRideDataSeries->datapoints[i]->number[wSpeedIndex];

                    // Speed stable for a certain period (2 seconds).
                    if (wConstantSpeedCount > (2 / wSampleRate))
                    {
                        int wNbPoints = 0;

                        // Rider is decelerating to a stop.
                        if (((wCurrentSpeed > 0) == false) && (wPreviousSpeed > 0))
                        {
                            // Smooth speed curve from the moment the speed was constant.
                            wNbPoints = std::max(4, wConstantSpeedCount);
                        }
                        // Rider starts moving from a stationary position.
                        else if ((wCurrentSpeed > 0) && ((wPreviousSpeed > 0) == false))
                        {
                            // Smooth speed curve from the moment the air pressure threshold has been reached.
                            wNbPoints = std::max(4, std::min(wConstantSpeedCount, wValidAirPressureCount));
                        }

                        // Interpolate speed.
                        if (wNbPoints > 0)
                        {
                            for (int k = 0; k < wNbPoints; k++)
                            {
                                double wNewSpeed = GcAlgo::MiscAlgo::interpolateSpeedSample(k, wNbPoints, wPreviousSpeed, wCurrentSpeed);

                                iRide->command->setXDataPointValue("RideData", (i - wNbPoints + k), wSpeedIndex + 2, wNewSpeed);
                                iRide->command->setPointValue((i - wNbPoints + k), RideFile::kph, wNewSpeed);
                            }
                        }
                    }

                    // Check if the current speed is valid and is equal to the previous speed.
                    if (((std::abs(wCurrentSpeed - wPreviousSpeed) > 0) == false) && (static_cast<int>(wRideDataSeries->datapoints[i]->number[wStatusByteIndex]) & 2))
                    {
                        wConstantSpeedCount++;

                        // Check if the pressure is over a certain threshold.
                        if (wRideDataSeries->datapoints[i]->number[wAirPressureIndex] > cAirPressureThreshold)
                        {
                            // Consecutive air pressure over threshold.
                            wValidAirPressureCount++;
                        }
                        else
                        {
                            wValidAirPressureCount = 0;
                        }
                    }
                    // Speed changed or is invalid.
                    else
                    {
                        wValidAirPressureCount = 0;
                        wConstantSpeedCount = 0;
                    }

                    wPreviousSpeed = wCurrentSpeed;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::restoreGpsData
///        This method restores the GPS data from the Garmin series in the case
///        where it is not available within Notio data.
///
/// \param[in] iRide    Ride file.
///////////////////////////////////////////////////////////////////////////////
void NotioComputeProcessor::restoreGpsData(RideFile *iRide)
{
    if (iRide)
    {
        // Get the Garmin data series.
        XDataSeries *wGarminSeries = iRide->xdata("GARMIN");
        XDataSeries *wRideDataSeries = iRide->xdata("RideData");

        // Check if GPS data is available after compute.
        bool wLatMissing = (iRide->isDataPresent(RideFile::lat) == false);
        bool wLonMissing = (iRide->isDataPresent(RideFile::lon) == false);

        // Restore missing GPS data.
        if (wGarminSeries && wRideDataSeries && (wLatMissing || wLonMissing))
        {
            int wLatitudeIndex = wGarminSeries->valuename.indexOf("gpsLatitude");
            int wLongitudeIndex = wGarminSeries->valuename.indexOf("gpsLongitude");
            int wHeadingIndex = wRideDataSeries->valuename.indexOf("heading");
            int wRideDataLatIndex = wRideDataSeries->valuename.indexOf("latitude");
            int wRideDataLonIndex = wRideDataSeries->valuename.indexOf("longitude");

            bool wLatDataPresent = false;
            bool wLonDataPresent = false;
            bool wFirstPass = true;

            double wPrevLat = 0.0, wPrevLon = 0.0, wHeading = 0.0;

            // Go through all data points.
            RideFileIterator it(iRide, Specification());
            while (it.hasNext())
            {
                // Get data point.
                RideFilePoint *wDataPoint = it.next();

                // Get the time index in the Garmin data series.
                int wTimeIndex = wGarminSeries->timeIndex(wDataPoint->secs);

                // Skip when time index out of bound.
                if ((wTimeIndex < 0) || (wTimeIndex >= wGarminSeries->datapoints.count()))
                    continue;

                // Get Garmin series data point.
                XDataPoint *wXdataPoint = wGarminSeries->datapoints[wTimeIndex];

                // Get and save GPS latitude.
                if ((wLatitudeIndex > -1) && (wRideDataLatIndex > -1) && wLatMissing)
                {
                    iRide->command->setPointValue(iRide->timeIndex(wDataPoint->secs), RideFile::lat, wXdataPoint->number[wLatitudeIndex]);
                    iRide->command->setXDataPointValue("RideData", iRide->timeIndex(wDataPoint->secs), wRideDataLatIndex + 2, wXdataPoint->number[wLatitudeIndex]);

                    // Validate if data present.
                    if (std::abs(wXdataPoint->number[wLatitudeIndex]) > 0.0)
                        wLatDataPresent = true;
                }

                // Get and save GPS longitude.
                if ((wLongitudeIndex > -1) && (wRideDataLonIndex > -1) && wLonMissing)
                {
                    iRide->command->setPointValue(iRide->timeIndex(wDataPoint->secs), RideFile::lon, wXdataPoint->number[wLongitudeIndex]);
                    iRide->command->setXDataPointValue("RideData", iRide->timeIndex(wDataPoint->secs), wRideDataLonIndex + 2, wXdataPoint->number[wLongitudeIndex]);

                    // Validate if data present.
                    if (std::abs(wXdataPoint->number[wLongitudeIndex]) > 0.0)
                        wLonDataPresent = true;
                }

                // Calculate heading since GPS data were missing.
                if ((wLatitudeIndex > -1) && (wLongitudeIndex > -1) && (wHeadingIndex > -1))
                {
                    double wLat = wXdataPoint->number[wLatitudeIndex];
                    double wLon = wXdataPoint->number[wLongitudeIndex];

                    if (wFirstPass)
                    {
                        wPrevLat = wLat;
                        wPrevLon = wLon;
                        wFirstPass = false;
                    }
                    else if ((fabs(wLat) > .0001) && (fabs(wLon) > .0001) && (fabs(wLat - wPrevLat) > .0000001) && (fabs(wLon - wPrevLon) > .0000001))
                    {
                        wHeading = MiscAlgo::calculateHeading(wPrevLat, wPrevLon, wLat, wLon);
                        wPrevLat = wLat;
                        wPrevLon = wLon;
                    }
                    iRide->command->setXDataPointValue("RideData", iRide->timeIndex(wDataPoint->secs), wHeadingIndex + 2, wHeading);
                }
            }

            // Set data present.
            iRide->command->setDataPresent(RideFile::lat, wLatDataPresent);
            iRide->command->setDataPresent(RideFile::lon, wLonDataPresent);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::getMissingDflyData
///        This method gets the missing Dfly data.
///
/// \param[in] iRide    Ride file.
///////////////////////////////////////////////////////////////////////////////
void NotioComputeProcessor::getMissingDflyData(RideFile *iRide)
{
    if (iRide)
    {
        // Get Dfly data from Gears series if available.
        XDataSeries *wGearsSeries = iRide->xdata("GEARS");
        XDataSeries *wRideDataSeries = iRide->xdata("RideData");
        if (wGearsSeries && wRideDataSeries)
        {
            int wFrontGearIndex = wGearsSeries->valuename.indexOf("frontNum");
            if (wFrontGearIndex == -1)
                wFrontGearIndex = wGearsSeries->valuename.indexOf("FRONT_NUM");

            if (wFrontGearIndex == -1)
                wFrontGearIndex = wGearsSeries->valuename.indexOf("FRONT-NUM");

            int wRearGearIndex = wGearsSeries->valuename.indexOf("rearNum");
            if (wRearGearIndex == -1)
                wRearGearIndex = wGearsSeries->valuename.indexOf("REAR_NUM");

            if (wRearGearIndex == -1)
                wRearGearIndex = wGearsSeries->valuename.indexOf("REAR-NUM");

            // Save front gear and rear gear values.
            if ((wFrontGearIndex > -1) && (wRearGearIndex > -1))
            {
                for (int i = 0; i < wRideDataSeries->datapoints.count(); i++)
                {
                    int wIndex = wGearsSeries->timeIndex(wRideDataSeries->datapoints[i]->secs);
                    double wFrontGear = wGearsSeries->datapoints[wIndex]->number[wFrontGearIndex];
                    double wRearGear = wGearsSeries->datapoints[wIndex]->number[wRearGearIndex];

                    iRide->command->setXDataPointValue("RideData", i, rideDataIdx::eFrontGear + 2, wFrontGear);
                    iRide->command->setXDataPointValue("RideData", i, rideDataIdx::eRearGear + 2, wRearGear);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioComputeProcessor::adjustAlt
///        This method calculates the computed altitude.
///
/// \param[in] ride     Current ride.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool NotioComputeProcessor::adjustAlt(RideFile *ride)
{
    // Create CDA XDataSeries
    XDataSeries *wRideDataSeries = ride->xdata("RideData");
    if (wRideDataSeries == nullptr)
        return false;

    vector_t acc;
    vector_t gyro;

    // Get XDataSeries column indexes.
    int wGyrXIndex = wRideDataSeries->valuename.indexOf("gyrX");
    int wGyrYIndex = wRideDataSeries->valuename.indexOf("gyrY");
    int wGyrZIndex = wRideDataSeries->valuename.indexOf("gyrZ");
    int wAccXIndex = wRideDataSeries->valuename.indexOf("accX");
    int wAccYIndex = wRideDataSeries->valuename.indexOf("accY");
    int wAccZIndex = wRideDataSeries->valuename.indexOf("accZ");
    int wPowerIndex = wRideDataSeries->valuename.indexOf("power");
    int wSpeedIndex = wRideDataSeries->valuename.indexOf("speed");
    int wAltIndex = wRideDataSeries->valuename.indexOf("alt");
    int wAltComputeIndex = wRideDataSeries->valuename.indexOf("altCompute");

    // Get input alitude.
    int numPoints = wRideDataSeries->datapoints.count();
    double *inArray = new double[static_cast<unsigned long>(numPoints)];
    double *outArray = new double[static_cast<unsigned long>(numPoints)];

    for ( int j = 0 ; j < numPoints ; j++ )
    {
        XDataPoint* xpoint = wRideDataSeries->datapoints.at(j);
        double xalt = xpoint->number[wAltIndex];;
        inArray[j] = xalt;
    }

    // Preprocess the altitude data.
    pre_process(numPoints, inArray, outArray);

    // Save processed data into computed altitude column.
    for (int j = 0 ; j < numPoints ; j++)
        ride->command->setXDataPointValue("RideData", j, wAltComputeIndex + 2, outArray[j] );

    // Free inArray.
    if (inArray)
    {
        delete [] inArray;
        inArray = nullptr;
    }

    // Free outArray.
    if (outArray)
    {
        delete [] outArray;
        outArray = nullptr;
    }

    // Slope estimation.
    slope_estimation_state_vector_t slope_estimation_state = { 0, 0, 0 };
    double error_phi = 0;
    double error_theta = 0;

    slope_estimation_init(&slope_estimation_state, error_phi, error_theta);

    for ( int j = 0 ; j < wRideDataSeries->datapoints.count() ; j++ )
    {
        XDataPoint* xpoint = wRideDataSeries->datapoints.at(j);

        double xalt = xpoint->number[wAltComputeIndex];
        double wSpeed = xpoint->number[wSpeedIndex];
        double wPower = xpoint->number[wPowerIndex];

        acc.x =  xpoint->number[wAccXIndex] / 1000.0 * 9.81;
        acc.y =  xpoint->number[wAccYIndex] / 1000.0 * 9.81;
        acc.z =  xpoint->number[wAccZIndex] / 1000.0 * 9.81;
        gyro.x = 1.0 * xpoint->number[wGyrXIndex] * M_PI / 180.0;
        gyro.y = 1.0 * xpoint->number[wGyrYIndex] * M_PI / 180.0;
        gyro.z = 1.0 * xpoint->number[wGyrZIndex] * M_PI / 180.0;

        slope_estimation_process(&acc, &gyro, xalt, wSpeed, wPower, &slope_estimation_state);

        // Adjust altitude.
        if (isnan(slope_estimation_state.h))
        {
            ride->command->setXDataPointValue("RideData", j, wAltComputeIndex + 2, 0.0);
            ride->command->setPointValue(j, RideFile::alt, 0.0);
        }
        else
        {
            ride->command->setXDataPointValue("RideData", j, wAltComputeIndex + 2, slope_estimation_state.h);
            ride->command->setPointValue(j, RideFile::alt, slope_estimation_state.h);
        }
    }
    return true;
}
