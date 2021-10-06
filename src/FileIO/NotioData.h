#ifndef NOTIODATA_H
#define NOTIODATA_H

#include <QVector>
#include <QStringList>

namespace NotioData {

// RideData XDataSeries columns.
typedef enum rideDataColumns { ePower, eSpeed, eAirPressure, eAirDensity,
                               eAccX, eAccY, eAccZ, eGyrX, eGyrY, eGyrZ,
                               eAlt, eAltCorrect, eAltGarmin, eAltCompute,
                               eHeading, eLatitude, eLongitude, eTemperature,
                               eHeartrate, eCadence, eFrontGear, eRearGear, eStatus
                             } rideDataIdx;

// RideData XDataSeries columns names.
static QStringList rideDataValuename = { "power", "speed", "airpressure", "airdensity",
                                         "accX", "accY", "accZ", "gyrX", "gyrY", "gyrZ",
                                         "alt", "altCorrect", "altGarmin", "altCompute",
                                         "heading", "latitude", "longitude", "temperature",
                                         "heartrate", "cadence", "frontGear", "rearGear", "status"
                                       };

// RideData XDataSeries columns units.
static QStringList rideDataUnitname = { "watts", "m/s", "", "kg/m³",
                                        "m/s²", "m/s²", "m/s²", "deg/s", "deg/s", "deg/s",
                                        "m", "m", "m", "m",
                                        "deg", "deg", "deg", "°C",
                                        "bpm", "rpm", "", "", ""
                                      };

// CdaData XDataSeries columns.
typedef enum cdaDataColumns { eBcvAlt, eEkfAlt, eHuAlt,
                              eETot, eECrr, eEAcc, eEAlt, eEDeH, eEAir,
                              eRawCDA, eEkfCDA,
                              eWind, eRoll, eVibration, eXChain,
                              // The following are to be removed in a future release.
                              // They are kept to avoid crashes on older versions of GC.
                              eSpeedLegacy, eAirdensity, eAirpressure, eLostCdA, eEBNO,
                              eAccCDA, eWinCDA, eRollCRR, eVibrationCRR,
                              eFrontGearLegacy, eRearGearLegacy, eWattsLost
                            } cdaDataIdx;

// RideData XDataSeries columns names.
static QStringList cdaDataValuename = { "bcvAlt", "ekfAlt", "huAlt",
                                        "eTot", "eCrr", "eAcc", "eAlt", "eDeH", "eAir",
                                        "rawCDA", "ekfCDA",
                                        "wind", "roll", "vibration", "XChain",
                                        // The following are to be removed in a future release.
                                        // They are kept to avoid crashes on older versions of GC.
                                        "speed", "airdensity", "airpressure", "lostCDA", "eBNO",
                                        "accCDA", "winCDA", "rollCRR", "vibrationCRR",
                                        "FrontGear", "RearGear", "WattsLost"
                                      };

// RideData XDataSeries columns units.
static QStringList cdaDataUnitname = { "m", "m", "m",
                                       "J", "m", "m²/s²", "m", "m", "J/(m²)",
                                       "m²", "m²",
                                       "m/s", "deg", "", "watts",
                                       // The following are to be removed in a future release.
                                       // They are kept to avoid crashes on older versions of GC.
                                       "m/s", "kg/m³", "", "", "J",
                                       "m²", "m²", "", "",
                                       "", "", ""
                                      };

///////////////////////////////////////////////////////////////////////////////
/// \brief The sRevCountAlgoVar struct
///        This structure defines the variables used in the distance calculation
///        bases on the revolution count of a speed sensor.
///////////////////////////////////////////////////////////////////////////////
struct sRevCountAlgoVar {
    unsigned int m_revCountBefore;
    unsigned int m_revCountActual;
    unsigned int m_revCountTotal;
    bool m_revCountWasZero;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The sRange struct
///        This structure defines a range of gears (front and rear) with their
///        associated power loss due to mechanical contrainst.
///////////////////////////////////////////////////////////////////////////////
struct sRange {
    int m_frontGear;
    int m_rearGear;
    double m_watts;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The eRideDataSensorStatus enum
///        This enum represents the sensors' statuses in the RideData XDataSeries.
///////////////////////////////////////////////////////////////////////////////
enum eRideDataSensorStatus {
    //    MSB                 LSB
    //    B15 = Provision     B7 = record status
    //    B14 = Provision     B6 = pause garmin
    //    B13 = Provision     B5 = pwr
    //    B12 = Provision     B4 = speed
    //    B11 = Provision     B3 = cad
    //    B10 = Provision     B2 = speedcad
    //    B9 = Provision      B1 = dfly
    //    B8 = Provision      B0 = Hrm
    cRecordStatus = 0x0080,
    cPauseGarminStatus = 0x0040,
    cPowerStatus = 0x0020,
    cSpeedStatus = 0x0010,
    cCadStatus = 0x0008,
    cSpeedCadStatus = 0x0004,
    cDflyStatus = 0x0002,
    cHrmStatus = 0x0001
};
}

#endif  // End of _NOTIODATA_H
