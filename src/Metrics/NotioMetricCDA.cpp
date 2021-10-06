#include "DataProcessor.h"
#include "LTMOutliers.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include "GcUpgrade.h"
#include "NotioData.h"
#include "IntervalItem.h"
#include "Specification.h"
#include "NotioComputeFunctions.h"
#include "AeroAlgo.h"

#include <QApplication>
#include <QHash>
#include <algorithm>
#include <QVector>
#include <iostream>

using namespace std;
using namespace NotioComputeFunctions;

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioPmEnergy class
///        This class defines the metric for total energy from power meter.
///////////////////////////////////////////////////////////////////////////////
class NotioPmEnergy : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NotioPmEnergy)

public:
    NotioPmEnergy();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const ;

    RideMetric *clone() const { return new NotioPmEnergy(*this); }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioPmEnergy::NotioPmEnergy
///        Constructor.
///////////////////////////////////////////////////////////////////////////////
NotioPmEnergy::NotioPmEnergy() {

    setSymbol("total_pm_energy");
    setInternalName("Total Power Meter Energy");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioPmEnergy::initialize
///        This method initializes the metric.
///////////////////////////////////////////////////////////////////////////////
void NotioPmEnergy::initialize()
{
    setPrecision(3);

    setName(tr("Total Power Meter Energy"));

    setMetricUnits("J");
    setImperialUnits("J");
    setType(RideMetric::Total);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("power");

    setDescription(tr("Total energy from the power meter."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioPmEnergy::compute
///        This method computes the total energy of the power for the
///        specification.
///
/// \param[in] item     Ride item pointer.
/// \param[in] iSpec    Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void NotioPmEnergy::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &)
{
    RideFile *wRideFile = item->ride();

    // No ride or no samples.
    if (wRideFile == nullptr)
    {
        setValue(RideFile::NIL);
        return;
    }

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        int wPowerIndex = xdataSeries->valuename.indexOf(XDataVariableName_);

        if (wPowerIndex < 0)
        {
            setValue(RideFile::NIL);
            return;
        }

        double wRecInt = NotioFuncCompute::estimateRecInterval(xdataSeries);
        double wMechEff = wRideFile->getTag("customMecEff", wRideFile->getTag("notio.efficiency", "1.0")).toDouble();
        double wJoules_pm_total = 0.0;

        // Calculate total energy.
        DataSeriesIterator it(xdataSeries, iSpec);
        while (it.hasNext())
        {
            XDataPoint *xpoint = it.next();
            wJoules_pm_total += (wMechEff * xpoint->number[wPowerIndex]) * wRecInt;
        }
        setValue(wJoules_pm_total);
    }
    else {
        setValue(RideFile::NIL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioPmEnergy::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool NotioPmEnergy::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr) {
        return false;
    }

    // We need power to calculate the energy.
    return const_cast<RideItem*>(item)->present.contains("P");
}

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioCrrEnergy class
///        This class defines the metric of the total rolling resistance energy
///        loss.
///////////////////////////////////////////////////////////////////////////////
class NotioCrrEnergy : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NotioCrrEnergy)

public:
    NotioCrrEnergy();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new NotioCrrEnergy(*this); }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCrrEnergy::NotioCrrEnergy
///        Constructor.
///////////////////////////////////////////////////////////////////////////////
NotioCrrEnergy::NotioCrrEnergy() {

    setSymbol("total_crr_energy");
    setInternalName("Total Rolling Resistance Energy");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCrrEnergy::initialize
///        This method initializes the metric.
///////////////////////////////////////////////////////////////////////////////
void NotioCrrEnergy::initialize()
{
    setPrecision(3);

    setName(tr("Total Rolling Resistance Energy"));

    setMetricUnits("J");
    setImperialUnits("J");
    setType(RideMetric::Total);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("speed");

    setDescription(tr("Total energy loss from rolling resistance."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCrrEnergy::compute
///        This method computes the total rolling resistance energy loss for
///        the specification.
///
/// \param[in] item     Ride item pointer.
/// \param[in] iSpec    Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void NotioCrrEnergy::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &)
{
    RideFile *wRideFile = item->ride();

    // No ride or no samples.
    if (wRideFile == nullptr)
    {
        setValue(RideFile::NIL);
        return;
    }

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        int wSpeedIndex = xdataSeries->valuename.indexOf(XDataVariableName_);

        if (wSpeedIndex < 0)
        {
            setValue(RideFile::NIL);
            return;
        }

        double wRecInt = NotioFuncCompute::estimateRecInterval(xdataSeries);
        double wTotalWeight = wRideFile->getTag("customWeight", wRideFile->getTag("notio.riderWeight", wRideFile->getTag("weight", "80"))).toDouble();
        double wCrr = wRideFile->getTag("customCrr", wRideFile->getTag("notio.crr", wRideFile->getTag("crr", "0.0040"))).toDouble();
        double wJoules_crr_total = 0.0;

        // Calculates the energy.
        DataSeriesIterator it(xdataSeries, iSpec);
        while (it.hasNext())
        {
            XDataPoint *xpoint = it.next();

            double wSpeed = xpoint->number[wSpeedIndex];
            wJoules_crr_total += GcAlgo::AeroAlgo::rolling_resistance_energy(wCrr, wTotalWeight, wSpeed, wRecInt);
        }
        setValue(wJoules_crr_total);
    }
    else {
        setValue(RideFile::NIL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCrrEnergy::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool NotioCrrEnergy::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr) {
        return false;
    }

    // We need speed to calculate the energy.
    return const_cast<RideItem*>(item)->present.contains("S");
}

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioAltEnergy class
///        This class defines the metric for the total potential energy
///        gain/loss.
///////////////////////////////////////////////////////////////////////////////
class NotioAltEnergy : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NotioAltEnergy)

public:
    NotioAltEnergy();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new NotioAltEnergy(*this); }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioAltEnergy::NotioAltEnergy
///        Constructor.
///////////////////////////////////////////////////////////////////////////////
NotioAltEnergy::NotioAltEnergy() {

    setSymbol("total_alt_energy");
    setInternalName("Total Potential Energy");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioAltEnergy::initialize
///        This method initializes the metric.
///////////////////////////////////////////////////////////////////////////////
void NotioAltEnergy::initialize()
{
    setPrecision(3);

    setName(tr("Total Potential Energy"));

    setMetricUnits("J");
    setImperialUnits("J");
    setType(RideMetric::Total);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("altCompute");

    setDescription(tr("Total energy gain/loss related to altitude."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioAltEnergy::compute
///        This method computes the total potential energy gain/loss for the
///        specification.
///
/// \param[in] item     Ride item pointer.
/// \param[in] iSpec    Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void NotioAltEnergy::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &)
{
    RideFile *wRideFile = item->ride();

    // No ride or no samples.
    if (wRideFile == nullptr)
    {
        setValue(RideFile::NIL);
        return;
    }

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        int wAltComputeIndex = xdataSeries->valuename.indexOf(XDataVariableName_);

        if (wAltComputeIndex < 0)
        {
            setValue(RideFile::NIL);
            return;
        }

        double wTotalWeight = wRideFile->getTag("customWeight", wRideFile->getTag("notio.riderWeight", wRideFile->getTag("weight", "80"))).toDouble();
        double wJoules_alt_total = 0.0;

        DataSeriesIterator it(xdataSeries, iSpec);
        XDataPoint *wPreviousPoint = xdataSeries->datapoints[std::max(it.firstIndex() - 1, 0)];

        // Calculate the energy.
        while (it.hasNext())
        {
            XDataPoint *xpoint = it.next();

            wJoules_alt_total += GcAlgo::AeroAlgo::potential_energy(wTotalWeight, xpoint->number[wAltComputeIndex], wPreviousPoint->number[wAltComputeIndex]);
            wPreviousPoint = xpoint;
        }
        setValue(wJoules_alt_total);
    }
    else {
        setValue(RideFile::NIL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioAltEnergy::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool NotioAltEnergy::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr) {
        return false;
    }

    // We need altitude to calculate energy.
    return const_cast<RideItem*>(item)->present.contains("A");
}

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioKineticEnergy class
///        This class defines the metric for the total kinetic energy gain/loss.
///////////////////////////////////////////////////////////////////////////////
class NotioKineticEnergy : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NotioKineticEnergy)

public:
    NotioKineticEnergy();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new NotioKineticEnergy(*this); }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioKineticEnergy::NotioKineticEnergy
///        Constructor.
///////////////////////////////////////////////////////////////////////////////
NotioKineticEnergy::NotioKineticEnergy() {

    setSymbol("total_kinetic_energy");
    setInternalName("Total Kinetic Energy");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioKineticEnergy::initialize
///        This method initializes the metric.
///////////////////////////////////////////////////////////////////////////////
void NotioKineticEnergy::initialize()
{
    setPrecision(3);

    setName(tr("Total Kinetic Energy"));

    setMetricUnits("J");
    setImperialUnits("J");
    setType(RideMetric::Total);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("speed");

    setDescription(tr("Total kinetic energy gain/loss."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioKineticEnergy::compute
///        This method computes the total kinetic energy gain/loss for the
///        specification.
///
/// \param[in] item     Ride item pointer.
/// \param[in] iSpec    Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void NotioKineticEnergy::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &)
{
    RideFile *wRideFile = item->ride();

    // No ride or no samples.
    if (wRideFile == nullptr)
    {
        setValue(RideFile::NIL);
        return;
    }

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        int wSpeedIndex = xdataSeries->valuename.indexOf(XDataVariableName_);

        if (wSpeedIndex < 0)
        {
            setValue(RideFile::NIL);
            return;
        }

        double wTotalWeight = wRideFile->getTag("customWeight", wRideFile->getTag("notio.riderWeight", wRideFile->getTag("weight", "80"))).toDouble();
        double wInertiaFactor = wRideFile->getTag("customInertia", wRideFile->getTag("notio.inertiaFactor", "1.15")).toDouble();
        double wJoules_inertia_total = 0.0;

        DataSeriesIterator it(xdataSeries, iSpec);
        XDataPoint *wPreviousPoint = xdataSeries->datapoints[std::max(it.firstIndex() - 1, 0)];

        // Calculate the energy.
        while (it.hasNext())
        {
            XDataPoint *xpoint = it.next();

            double wSpeed = xpoint->number[wSpeedIndex];
            double wPrevSpeed = wPreviousPoint->number[wSpeedIndex];

            wJoules_inertia_total += GcAlgo::AeroAlgo::kinetic_energy(wInertiaFactor, wTotalWeight, wSpeed, wPrevSpeed);
            wPreviousPoint = xpoint;
        }
        setValue(wJoules_inertia_total);
    }
    else {
        setValue(RideFile::NIL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioKineticEnergy::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool NotioKineticEnergy::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr) {
        return false;
    }

    // We need speed to calculate energy.
    return const_cast<RideItem*>(item)->present.contains("S");
}

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioCdaEnergy class
///        This class defines the metric for the total aerodynamic energy per
///        square meter.
///////////////////////////////////////////////////////////////////////////////
class NotioCdaEnergy : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(NotioCdaEnergy)

public:
    NotioCdaEnergy();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new NotioCdaEnergy(*this); }

private:
    QString m_secondXDataVariableName_;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCdaEnergy::NotioCdaEnergy
///        Constructor.
///////////////////////////////////////////////////////////////////////////////
NotioCdaEnergy::NotioCdaEnergy() {

    setSymbol("total_cda_energy");
    setInternalName("Total Aerodynamic Energy");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCdaEnergy::initialize
///        This method initializes the metric.
///////////////////////////////////////////////////////////////////////////////
void NotioCdaEnergy::initialize()
{
    setPrecision(3);

    setName(tr("Total Aerodynamic Energy"));

    setMetricUnits("J/m²");
    setImperialUnits("J/m²");
    setType(RideMetric::Total);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("airpressure");
    m_secondXDataVariableName_ = "speed";

    setDescription(tr("Total aerodynamic energy per square meter."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCdaEnergy::compute
///        This method computes the total aerodynamic energy per square meter
///        for the specification.
///
/// \param[in] item     Ride item pointer.
/// \param[in] iSpec    Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void NotioCdaEnergy::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &)
{
    RideFile *wRideFile = item->ride();

    // No ride or no samples.
    if (wRideFile == nullptr)
    {
        setValue(RideFile::NIL);
        return;
    }

    double wRiderFactor = wRideFile->getTag("customRiderFactor", wRideFile->getTag("notio.riderFactor", "1.39")).toDouble();
    double wRiderExponent = wRideFile->getTag("customExponent", wRideFile->getTag("notio.riderExponent", "-0.05")).toDouble();

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        int wAirPressureIndex = xdataSeries->valuename.indexOf(XDataVariableName_);
        int wSpeedIndex = xdataSeries->valuename.indexOf(m_secondXDataVariableName_);

        if ((wAirPressureIndex < 0) || (wSpeedIndex < 0))
        {
            setValue(RideFile::NIL);
            return;
        }

        bool wOldFormat = (item->ride()->getTag("Gc Min Version", "0").toInt() <= NK_VERSION_LATEST);

        double wRecInt = NotioFuncCompute::estimateRecInterval(xdataSeries);
        double wJoules_cda_total = 0.0;

        // Calculate energy.
        DataSeriesIterator it(xdataSeries, iSpec);
        while (it.hasNext())
        {
            XDataPoint *xpoint = it.next();

            double wAirPress = xpoint->number[wAirPressureIndex] / (wOldFormat ? GcAlgo::AeroAlgo::cAirPressureSensorFactor : 1.0);
            double wSpeed = xpoint->number[wSpeedIndex];

            wJoules_cda_total += GcAlgo::AeroAlgo::aero_energy(wRiderFactor, wRiderExponent, wAirPress, wSpeed, wRecInt);
        }
        setValue(wJoules_cda_total);
    }
    else {
        setValue(RideFile::NIL);
        setCount(0.0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool NotioCdaEnergy::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr)
        return false;

    // We need speed and wind to calculate energy.
    return (const_cast<RideItem*>(item)->present.contains("S") /*&& const_cast<RideItem*>(item)->present.contains("W")*/);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief The xCDA class
///        This class defines the metric representing the average CdA.
///////////////////////////////////////////////////////////////////////////////
class xCDA : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(xCDA)

public:
    xCDA();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &iDeps);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new xCDA(*this); }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief xCDA::xCDA
///        Constructor
///////////////////////////////////////////////////////////////////////////////
xCDA::xCDA() {

    setSymbol("CDA");
    setInternalName("Average CdA");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xCDA::initialize
///        This method initializes the metric object.
///////////////////////////////////////////////////////////////////////////////
void xCDA::initialize()
{
    setPrecision(3);

    setName(tr("Average CdA"));

    setMetricUnits("m²");
    setImperialUnits("m²");
    setType(RideMetric::Average);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("");

    setDescription(tr("Average CdA calculated from data captured with a Notio device."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xCDA::compute
///        This method computes the average value based on specification.
///
/// \param[in] item     Current RideItem pointer.
/// \param[in] spec     Specification for the calculus.
/// \param[in] iDeps    Metric dependencies.
/// ////////////////////////////////////////////////////////////////////////////
void xCDA::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &iDeps)
{
    Q_UNUSED(iSpec)

    RideFile *wRideFile = item->ride();

    // no ride or no samples
    if (wRideFile == nullptr)
    {
        setValue(RideFile::NIL);
        return;
    }

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        if ((iDeps.value("total_pm_energy") == nullptr) || (iDeps.value("total_crr_energy") == nullptr) ||
                (iDeps.value("total_alt_energy") == nullptr) || (iDeps.value("total_kinetic_energy") == nullptr) ||
                (iDeps.value("total_cda_energy") == nullptr))
        {
            setValue(RideFile::NIL);
            return;
        }

        double wPmEnergy = iDeps.value("total_pm_energy")->value(true);
        double wCrrEnergy = iDeps.value("total_crr_energy")->value(true);
        double wAltEnergy = iDeps.value("total_alt_energy")->value(true);
        double wInertiaEnergy = iDeps.value("total_kinetic_energy")->value(true);
        double wCdAEnergy = iDeps.value("total_cda_energy")->value(true);

        double wCda = GcAlgo::AeroAlgo::calculate_cda(wCdAEnergy, wPmEnergy, wAltEnergy, wInertiaEnergy, wCrrEnergy);

        setValue(wCda);
    }
    else {
        setValue(RideFile::NIL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xCDA::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool xCDA::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need power and speed to calculate CdA.
    return (const_cast<RideItem*>(item)->present.contains("P") && const_cast<RideItem*>(item)->present.contains("S") &&
            RideMetricFactory::instance().rideMetric("total_pm_energy")->isRelevantForRide(item) &&
            RideMetricFactory::instance().rideMetric("total_crr_energy")->isRelevantForRide(item) &&
            RideMetricFactory::instance().rideMetric("total_alt_energy")->isRelevantForRide(item) &&
            RideMetricFactory::instance().rideMetric("total_kinetic_energy")->isRelevantForRide(item) &&
            RideMetricFactory::instance().rideMetric("total_cda_energy")->isRelevantForRide(item));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief addCdAMetric
///        This function add the CdA metrics to the Ride Metric Factory.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
static bool addCdAMetrics()
{
    // Add energy metrics.
    RideMetricFactory::instance().addMetric(NotioPmEnergy());
    RideMetricFactory::instance().addMetric(NotioCrrEnergy());
    RideMetricFactory::instance().addMetric(NotioAltEnergy());
    RideMetricFactory::instance().addMetric(NotioKineticEnergy());
    RideMetricFactory::instance().addMetric(NotioCdaEnergy());

    // Set CdA metric dependencies.
    QVector<QString> wDeps;
    wDeps.append("total_pm_energy");
    wDeps.append("total_crr_energy");
    wDeps.append("total_alt_energy");
    wDeps.append("total_kinetic_energy");
    wDeps.append("total_cda_energy");

    RideMetricFactory::instance().addMetric(xCDA(), &wDeps);
    return true;
}

// Add CdA metric.
static bool xCDA_NKAdded = addCdAMetrics();
