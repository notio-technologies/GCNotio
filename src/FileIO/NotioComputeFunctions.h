#ifndef NOTIOCOMPUTEFUNCTIONS_H
#define NOTIOCOMPUTEFUNCTIONS_H

#include "NotioData.h"

class XDataPoint;
class XDataSeries;
class RideItem;
class RideFile;

namespace NotioComputeFunctions {

class NotioFuncCompute
{
public:
    NotioFuncCompute(RideItem *iRideItem) : m_ride(iRideItem)  {}
    ~NotioFuncCompute() {}

    static constexpr double cAltConstant = 10;

    int operator()(QString iType = "COMPUTE");

    // Global data series methods.
    static double estimateRecInterval(XDataSeries *iSeries);
    static bool speedPowerUnavailable(RideFile *iRide, QString &oMessage);

    // Global samples methods.
    static double calculateDist(const unsigned int iSampleRevCount, const double iTireSize, NotioData::sRevCountAlgoVar &ioRevCount);
    static double calculateAirDensity(double iTemperature, double iHumidity, double iAtmPressure);

    // CDAData data series related methods.
    static double findGearPowerLost(const int &iFrontGear, const int &iRearGear);
private:
    RideItem *m_ride;
};
}

#endif // NOTIOCOMPUTEFUNCTIONS_H
