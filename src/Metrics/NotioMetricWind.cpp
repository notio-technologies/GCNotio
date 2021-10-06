#include "DataProcessor.h"
#include "NotioComputeFunctions.h"
#include "AeroAlgo.h"
#include "LTMOutliers.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include "GcUpgrade.h"
#include "RideMetric.h"
#include "RideItem.h"
#include "Specification.h"

#include <QApplication>
#include <QHash>
#include <algorithm>
#include <QVector>
#include <iostream>

using namespace std;
using namespace NotioComputeFunctions;

///////////////////////////////////////////////////////////////////////////////
/// \brief The xWindSpeed class
///        This class defines the metric representing the average wind speed.
///////////////////////////////////////////////////////////////////////////////
class xWindSpeed : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(xWindSpeed)

public:
    xWindSpeed();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new xWindSpeed(*this); }
};

///////////////////////////////////////////////////////////////////////////////
/// \brief xWindSpeed::xWindSpeed
///        Constructor
///////////////////////////////////////////////////////////////////////////////
xWindSpeed::xWindSpeed() {

    setSymbol("Wind");
    setInternalName("Average Wind Speed");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xWindSpeed::initialize
///        This method initializes the metric object.
///////////////////////////////////////////////////////////////////////////////
void xWindSpeed::initialize()
{
    setPrecision(1);

    setName(tr("Average Wind Speed"));

    setMetricUnits(tr("kph"));
    setImperialUnits(tr("mph"));
    setConversion(static_cast<double>(MILES_PER_KM));
    setType(RideMetric::Average);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("airpressure");

    setDescription(tr("Average wind speed calculated from data captured with a Notio device."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xWindSpeed::compute
///        This method computes the average value based on specification.
///
/// \param[in] item Current RideItem pointer.
/// \param[in] spec Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void xWindSpeed::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &) {

    double counter = 0.0;
    double sum = 0.0;

    RideFile *wRideFile = item->ride();

    // No ride or no samples
    if (wRideFile == nullptr) {
        setValue(RideFile::NIL);
        setCount(0.0);
        return;
    }

    double wRiderFactor = wRideFile->getTag("customRiderFactor", wRideFile->getTag("notio.riderFactor", "1.39")).toDouble();
    double wRiderExponent = wRideFile->getTag("customExponent", wRideFile->getTag("notio.riderExponent", "-0.05")).toDouble();

    // Get the XData series pointer.
    XDataSeries *xdataSeries = wRideFile->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        // Get the wind data index.
        int varIndex = xdataSeries->valuename.indexOf(XDataVariableName_);
        int wAirDensityIndex = xdataSeries->valuename.indexOf("airdensity");
        int wSpeedIndex = xdataSeries->valuename.indexOf("speed");

        if ((varIndex < 0) || (wAirDensityIndex < 0) || (wSpeedIndex < 0))
        {
            setValue(RideFile::NIL);
            setCount(0.0);
            return;
        }

        bool wOldFormat = (item->ride()->getTag("Gc Min Version", "0").toInt() <= NK_VERSION_LATEST);

        // Calculates the metric's average.
        DataSeriesIterator it(xdataSeries, iSpec);

        while (it.hasNext()) {
            XDataPoint *xpoint = it.next();
            counter++;

            // Calculate head wind.
            double wAirPressure = std::max(xpoint->number[varIndex] / (wOldFormat ? GcAlgo::AeroAlgo::cAirPressureSensorFactor : 1.0), 0.0);
            double wRho = xpoint->number[wAirDensityIndex];
            double wHeadwind = GcAlgo::AeroAlgo::calculateHeadwind(wRiderFactor, wRiderExponent, wAirPressure, wRho);

            // Get real wind value.
            double wWind = 0.0;
            if (wAirPressure > 0.0)
                wWind = wHeadwind - xpoint->number[wSpeedIndex] * GcAlgo::Utility::cMeterPerSecToKph;

            sum += wWind;
        }

        double x = 0.0;
        if (counter > 0)
            x = sum / counter;

        setValue(x);
        setCount(counter);
    }
    else {
        setValue(RideFile::NIL);
        setCount(0.0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xWindSpeed::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool xWindSpeed::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr) {
        return false;
    }

    // To extract wind speed, we need to know our speed.
    return const_cast<RideItem*>(item)->present.contains("S");
}

///////////////////////////////////////////////////////////////////////////////
/// \brief addMetric
///        This function add the wind metric to the Ride Metric Factory.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
static bool addMetric()
{
    RideMetricFactory::instance().addMetric(xWindSpeed());
    return true;
}

static bool xWindSpeed_NKAdded = addMetric();
