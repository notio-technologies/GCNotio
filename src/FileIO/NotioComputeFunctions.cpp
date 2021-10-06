#include "NotioComputeFunctions.h"
#include "DataProcessor.h"
#include "MainWindow.h"
#include "Settings.h"
#include <math.h>

using namespace std;
using namespace NotioData;

namespace NotioComputeFunctions {

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioFuncCompute::operator ()
///        This method computes the current ride to be able to analize Notio
///        data and find CdA.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
int NotioFuncCompute::operator()(QString iType)
{
    int wReturning = 0;
    if (m_ride)
    {
        DataProcessor *wComputeProcessor = DataProcessorFactory::instance().getProcessors().find("_Compute Notio Data").value();

        if (wComputeProcessor)
        {
            wReturning = wComputeProcessor->postProcess(m_ride->ride(), nullptr, iType);
        }
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioFuncCompute::speedPowerUnavailable
///        This method checks if speed sensor or power meter data are missing
///        by calculating the sum of each sensor data values.
///
/// \param[in]  iRide       Current ride to compute.
/// \param[out] oMessage    Error message to show user.
///
/// \return The result of sensors data availability.
///////////////////////////////////////////////////////////////////////////////
bool NotioFuncCompute::speedPowerUnavailable(RideFile *iRide, QString &oMessage)
{
    bool wReturning = true;
    if (iRide)
    {
        XDataSeries *wBcvxSeries = iRide->xdata("BCVX");

        if (wBcvxSeries)
        {
            int wSpeedIndex = wBcvxSeries->valuename.indexOf("speed");
            int wPowerIndex = wBcvxSeries->valuename.indexOf("power");
            double wSpeedSum = 0.0;
            double wPowerSum = 0.0;

            // Need both power and speed sensors data.
            if ((wSpeedIndex != -1) && (wPowerIndex != -1))
            {
                // Calculate sum of each sensor data. A sum of 0 indicates there are no data.
                for (auto &wXdataPoint : wBcvxSeries->datapoints)
                {
                    wSpeedSum += wXdataPoint->number[wSpeedIndex];
                    wPowerSum += wXdataPoint->number[wPowerIndex];
                }
            }

            // Both power and speed sensors data are missing.
            if (((wSpeedSum > 0.0) == false) && ((wPowerSum > 0.0) == false))
                oMessage = QObject::tr("We can't compute without power meter and speed sensor data. You must use a power meter and a speed sensor.");
            // Speed sensor data is missing.
            else if ((wSpeedSum > 0.0) == false)
                oMessage = QObject::tr("We can't compute without speed sensor data. As GPS speed is not accurate enough, you must use a speed sensor.");
            // Power meter data is missing.
            else if ((wPowerSum > 0.0) == false)
                oMessage = QObject::tr("We can't compute without power meter data. You must use a power meter.");
            else
                wReturning = false;
        }
        // BCVX data is missing.
        else {
            oMessage = QObject::tr("Notio raw data (BCVX) missing.");
        }
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioFuncCompute::calculateDist
///        This method calculates the distance based on the revolution count
///        coming from the speed sensor.
///
/// \param[in] iSampleRevCount  Current sample rev count.
/// \param[in] iTireSize        Tire circumference.
/// \param[in, out] ioRevCount  Struct containing variables used for calculation.
///
/// \return The calculated distance in meters.
///////////////////////////////////////////////////////////////////////////////
double NotioFuncCompute::calculateDist(const unsigned int iSampleRevCount, const double iTireSize, sRevCountAlgoVar &ioRevCount)
{
    if (iSampleRevCount > 0)
    {
        ioRevCount.m_revCountBefore = ioRevCount.m_revCountActual;
        ioRevCount.m_revCountActual = iSampleRevCount;
        if (ioRevCount.m_revCountBefore > 0)
        {
            if (ioRevCount.m_revCountActual > ioRevCount.m_revCountBefore)
            {
                ioRevCount.m_revCountTotal += ioRevCount.m_revCountActual - ioRevCount.m_revCountBefore;
            }
            else if (ioRevCount.m_revCountActual < ioRevCount.m_revCountBefore)
            {
                // Roll over
                if ((ioRevCount.m_revCountWasZero) && (ioRevCount.m_revCountBefore < 65450))
                {
                    if ((std::abs(static_cast<double>(ioRevCount.m_revCountActual - ioRevCount.m_revCountBefore)) * iTireSize) > 750)
                    {
                        ioRevCount.m_revCountTotal += ioRevCount.m_revCountTotal; //to protect against some speed sensor not return the good rev count after a 0
                    }
                    else
                        ioRevCount.m_revCountTotal += ioRevCount.m_revCountActual;
                }
                else
                    ioRevCount.m_revCountTotal += ioRevCount.m_revCountActual + (65535 - ioRevCount.m_revCountBefore);
            }
        }
        ioRevCount.m_revCountWasZero = false;
    }
    else
        ioRevCount.m_revCountWasZero = true;

    // Return distance in meters.
    return iTireSize * static_cast<double>(ioRevCount.m_revCountTotal) / 1000;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief AddIntervalDialog::calculateAirDensity
///        This method calculates the air density.
///
///        Reference: https://www.holsoft.nl/physics/ocmain.htm
///
/// \param[in] iTemperature Temperature value.
/// \param[in] iHumidity    Humidity value.
/// \param[in] iAtmPressure Atmospheric pressure value.
///
/// \return The air density value.
///////////////////////////////////////////////////////////////////////////////
double NotioFuncCompute::calculateAirDensity(double iTemperature, double iHumidity, double iAtmPressure)
{
    double wPsat = 610.7 * pow(10, (7.5 * iTemperature) / (237.3 + iTemperature));
    double wRdry = 1.2929 * 273.15 / (iTemperature + 273.15);
    double wRwet = wRdry * (iAtmPressure - 0.3783 * iHumidity * wPsat / 100.0) / 1.01325E5;

    return (wRwet * 1000.0 + 0.5) / 1000.0;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioFuncCompute::findGearPowerLost
///        This method finds the power lost from gear mechanism.
///
/// \param[in] iFrontGear   Front gear number.
/// \param[in] iRearGear    Rear gear number.
///
/// \return The power value in watts.
///////////////////////////////////////////////////////////////////////////////
double NotioFuncCompute::findGearPowerLost(const int &iFrontGear, const int &iRearGear)
{
    // Define multiple ranges of gears and their associated power loss.
    static const NotioData::sRange ranges[] = {
        { 1, 1,  9.80 },
        { 1, 2,  9.05 },
        { 1, 3,  8.65 },
        { 1, 4,  8.00 },
        { 1, 5,  7.40 },
        { 1, 6,  6.85 },
        { 1, 7,  6.90 },
        { 1, 8,  6.97 },
        { 1, 9,  7.20 },
        { 1, 10, 7.35 },
        { 1, 11, 7.40 },
        { 2, 1,  8.75 },
        { 2, 2,  8.05 },
        { 2, 3,  7.50 },
        { 2, 4,  7.00 },
        { 2, 5,  7.00 },
        { 2, 6,  7.10 },
        { 2, 7,  7.10 },
        { 2, 8,  7.15 },
        { 2, 9,  7.25 },
        { 2, 10, 7.45 },
        { 2, 11, 7.75 },
    };

    // Number of gears' ranges defined.
    int numRanges = sizeof(ranges)/sizeof(NotioData::sRange);

    for (int i = 0 ; i < numRanges ; i++ )
    {
        if ((iFrontGear == ranges[i].m_frontGear) && (iRearGear == ranges[i].m_rearGear))
        {
            return ranges[i].m_watts;
        }
    }
    return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioFuncCompute::estimateRecInterval
///        This method estimates the recording interval of a XDataSeries.
///
/// \param[in] iSeries XDataSeries.
/// \return The recording interval of the data series.
///////////////////////////////////////////////////////////////////////////////
double NotioFuncCompute::estimateRecInterval(XDataSeries *iSeries)
{
    double wReturning = 1.0;

    if (iSeries)
    {
        int n = iSeries->datapoints.size();
        n = qMin(n, 1000);
        if (n >= 2)
        {
            QVector<double> secs(n-1);
            for (int i = 0; i < n-1; ++i) {
                double now = iSeries->datapoints[i]->secs;
                double then = iSeries->datapoints[i+1]->secs;
                secs[i] = then - now;
            }
            std::sort(secs.begin(), secs.end());
            int mid = n / 2 - 1;
            wReturning = round(secs[mid] * 1000.0) / 1000.0;
        }
    }
    return wReturning;
}
}
