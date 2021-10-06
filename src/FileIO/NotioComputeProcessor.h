#ifndef NOTIOCOMPUTEPROCESSOR_H
#define NOTIOCOMPUTEPROCESSOR_H

#include "NotioData.h"
#include "AeroAlgo.h"
#include "DataProcessor.h"
#include "HelpWhatsThis.h"
#include "Context.h"

#include <QThread>
#include <QObject>
#include <QWidget>
#include <QProgressDialog>

class NotioComputeProcessor;

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioComputeProcessorConfig class
///        This class defines the configuration widget used by the Preferences/
///        Options config panes for "Compute Notio Data" data processor.
///////////////////////////////////////////////////////////////////////////////
class NotioComputeProcessorConfig: public DataProcessorConfig {
    Q_DECLARE_TR_FUNCTIONS(NotioComputeProcessorConfig)

    friend class ::NotioComputeProcessor;
protected:

    QHBoxLayout *m_layout = nullptr;

    QLabel *m_speedOffsetLabel = nullptr;
    QDoubleSpinBox *m_speedOffsetDoubleSpinBox = nullptr;
    QLabel *m_wheelLabel = nullptr;
    QSpinBox *m_wheelSpinBox = nullptr;
    QLabel *m_altLabel = nullptr;
    QComboBox *m_altComboBox = nullptr;
    QLabel *m_weightLabel = nullptr;
    QDoubleSpinBox *m_weightDoubleSpinBox = nullptr;
    QLabel *m_crrLabel = nullptr;
    QDoubleSpinBox *m_crrDoubleSpinBox = nullptr;
    QLabel *m_riderFactorLabel = nullptr;
    QDoubleSpinBox *m_riderFactorDoubleSpinBox = nullptr;
    QLabel *m_mecEffLabel = nullptr;
    QDoubleSpinBox *m_mecEffDoubleSpinBox = nullptr;

    // Development parameters
    QLabel *m_riderExponentLabel = nullptr;
    QDoubleSpinBox *m_riderExponentDoubleSpinBox = nullptr;
    QLabel *m_inertiaFactorLabel = nullptr;
    QDoubleSpinBox *m_inertiaFactorDoubleSpinBox = nullptr;

public:
    // Constructor
    NotioComputeProcessorConfig(QWidget *iParent);

    //~NotioComputeProcessorConfig() {} // deliberately not declared since Qt will delete
    // the widget and its children when the config pane is deleted

    // Descriptive text within the configuration dialog window.
    QString explain();

    // Read data processor parameters.
    void readConfig() {
        m_wheelSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_WHEEL_SIZE, 2136).toInt());
        m_weightDoubleSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_TOTAL_WEIGHT, 80).toDouble());
        m_riderFactorDoubleSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_RIDER_FACTOR, 1.39).toDouble());
        m_crrDoubleSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_CRR, 0.004).toDouble());
        m_mecEffDoubleSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_MECH_EFF, 1.0).toDouble());
        m_altComboBox->setCurrentIndex(appsettings->value(nullptr, GC_NOTIO_DATA_ALT_TYPE, 0).toInt());    // Default corrected altitude.
        m_speedOffsetDoubleSpinBox->setValue(0.0); // Always put an offset of 0 seconds.

        // Development parameters
        m_riderExponentDoubleSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_RIDER_EXPONENT, -0.05).toDouble());
        m_inertiaFactorDoubleSpinBox->setValue(appsettings->value(nullptr, GC_NOTIO_DATA_INERTIA_FACTOR, 1.15).toDouble());
    }

    // Save data processor parameters.
    void saveConfig() {
        appsettings->setValue(GC_NOTIO_DATA_WHEEL_SIZE, m_wheelSpinBox->value());
        appsettings->setValue(GC_NOTIO_DATA_ALT_TYPE, m_altComboBox->currentIndex());
        appsettings->setValue(GC_NOTIO_DATA_TOTAL_WEIGHT, m_weightDoubleSpinBox->value());
        appsettings->setValue(GC_NOTIO_DATA_RIDER_FACTOR, m_riderFactorDoubleSpinBox->value());
        appsettings->setValue(GC_NOTIO_DATA_CRR, m_crrDoubleSpinBox->value());
        appsettings->setValue(GC_NOTIO_DATA_MECH_EFF, m_mecEffDoubleSpinBox->value());

        // Development parameters
        appsettings->setValue(GC_NOTIO_DATA_RIDER_EXPONENT, m_riderExponentDoubleSpinBox->value());
        appsettings->setValue(GC_NOTIO_DATA_INERTIA_FACTOR, m_inertiaFactorDoubleSpinBox->value());
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioComputeProcessor class
///        This class defines a data processor used to compute an activity
///        Notio data.
///////////////////////////////////////////////////////////////////////////////
class NotioComputeProcessor : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(NotioComputeProcessor)

public:
    NotioComputeProcessor() {}
    ~NotioComputeProcessor() {}

    // The processor.
    bool postProcess(RideFile *iRide, DataProcessorConfig *iConfig = nullptr, QString iOp = "");

    // The config widget.
    DataProcessorConfig* processorConfig(QWidget *iParent) {
        return new NotioComputeProcessorConfig(iParent);
    }

    // Localized Name.
    QString name() {
        return (tr("Compute Notio Data"));
    }

private:
    static constexpr int cMaxProgress = 5;
    enum eSteps { INIT, LOAD_RIDEDATA, LOAD_CDADATA, SAVE, REFRESH, NB_STEPS };

    // Ride pre process methods.
    bool convertRow2Bcvx(RideFile *iRide);
    void loadConfiguration(RideFile *iRide, DataProcessorConfig *iConfig);

    // RideData data series related methods.
    bool loadNotioData(RideFile *iRide);
    QVector<double> preCalculateDist(XDataSeries *iBcvxSeries, const double &iSampleRate, const int &iSampleOffset, const int &iTireSize, const double &iSpeedCorrection);

    // CdA data series related methods.
    void loadDisplayData(RideFile *iRide);
    void computeCda2Display(RideFile *iRide, GcAlgo::AeroAlgo::sEnergiesVectors &iEnergies);

    // RideData data series post process methods.
    void smoothSpeedCurve(RideFile *iRide);
    void restoreGpsData(RideFile *iRide);
    void getMissingDflyData(RideFile *iRide);
    bool adjustAlt(RideFile *ride);

    inline double totalJoules(QVector<double> ioStorage) { return std::accumulate(ioStorage.begin(), ioStorage.end(), 0.0); }
};

// Register data processor.
static bool NotioComputeProcessorAdded =
        DataProcessorFactory::instance().registerProcessor(
            QString("_Compute Notio Data"), new NotioComputeProcessor());

#endif // NOTIOCOMPUTEPROCESSOR_H
