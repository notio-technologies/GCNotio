#include "DataProcessor.h"
#include "LTMOutliers.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include "GcUpgrade.h"
#include <algorithm>
#include <QVector>
#include <iostream>

using namespace std;

#include "NotioData.h"
#include "Specification.h"
#include <QApplication>
#include <QHash>

///////////////////////////////////////////////////////////////////////////////
/// \brief The xAirPressure class
///        This class defines the metric representing the average differential
///        air pressure measured by the pitot tube part of the Notio device.
///////////////////////////////////////////////////////////////////////////////
class xAirPressure : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(xAirPressure)

private:
    double count = 0.0;

public:
    xAirPressure();

    void initialize();

    void compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &);

    bool isRelevantForRide(const RideItem *item) const;

    RideMetric *clone() const { return new xAirPressure(*this); }

    static constexpr double PSI_PER_PA = 0.0001450377;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief xAirPressure::xAirPressure
///        Constructor
///////////////////////////////////////////////////////////////////////////////
xAirPressure::xAirPressure() {
    setSymbol("AirPressure");
    setInternalName("Average Pitot Pressure");
    setSourceType(RideMetric::Notio);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xAirPressure::initialize
///        This method initializes the metric object.
///////////////////////////////////////////////////////////////////////////////
void xAirPressure::initialize()
{
    setPrecision(3);

    setName(tr("Average Pitot Pressure"));

    setMetricUnits(tr("Pa"));
    setImperialUnits(tr("psi"));
    setType(RideMetric::Average);
    setConversion(PSI_PER_PA);

    // Set XData information for Notio devices.
    setXDataSeriesName("RideData");
    setXDataVariableName("airpressure");

    setDescription(tr("Average of the differential pressure measured by the Notio's pitot tube."));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xAirPressure::compute
///        This method computes the average value based on specification.
///
/// \param[in] item Current RideItem pointer.
/// \param[in] spec Specification for the calculus.
///////////////////////////////////////////////////////////////////////////////
void xAirPressure::compute(RideItem *item, Specification iSpec, const QHash<QString,RideMetric*> &) {

    double sum = 0.0;
    count = 0.0;

    // No ride or no samples
    if (item->ride() == nullptr) {
        setValue(RideFile::NIL);
        setCount(0);
        return;
    }

    // Get the XData series pointer.
    XDataSeries *xdataSeries = item->ride()->xdata(xDataSeriesName_);
    if (xdataSeries)
    {
        // Get the differential air pressure data index.
        int varIndex = xdataSeries->valuename.indexOf(XDataVariableName_);

        if (varIndex < 0 ) {
            setValue(RideFile::NIL);
            setCount(0.0);
            return;
        }

        bool wOldFormat = (item->ride()->getTag("Gc Min Version", "0").toInt() <= NK_VERSION_LATEST);

        // Calculates the metric's average.
        DataSeriesIterator it(xdataSeries, iSpec);

        while (it.hasNext()) {
            XDataPoint *xpoint = it.next();

            count++;
            sum += xpoint->number[varIndex] / (wOldFormat ? 120.0 : 1.0);
        }

        if (count > 0)
            sum /= count;

        setValue(sum);
        setCount(count);
    }
    else {
        setValue(RideFile::NIL);
        setCount(0.0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief xAirPressure::isRelevantForRide
///        This method is called to determine if the metric is relevant for the
///        ride.
///
/// \param[in] item Current RideItem pointer.
/// \return The relevance of the metric for the ride.
///////////////////////////////////////////////////////////////////////////////
bool xAirPressure::isRelevantForRide(const RideItem *item) const {

    if ((item == nullptr) || (const_cast<RideItem*>(item)->ride() == nullptr)) {
        return false;
    }

    // We need xdata.
    if (const_cast<RideItem*>(item)->ride()->xdata(xDataSeriesName_) == nullptr) {
        return false;
    }

    // We need to be moving.
    return (const_cast<RideItem*>(item)->present.contains("S") || const_cast<RideItem*>(item)->present.contains("D"));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief addMetric
///        This function add the air pressure metric to the Ride Metric Factory.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
static bool addMetric()
{
    RideMetricFactory::instance().addMetric(xAirPressure());
    return true;
}

static bool xAirPressure_NKAdded = addMetric();
