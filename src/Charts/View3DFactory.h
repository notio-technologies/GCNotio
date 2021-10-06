/*
 * Copyright (c) 2017 Ahmed Id-Oumohmed
 *               2018 Michaël Beaulieu (michael.beaulieu@notiotechnologies.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef VIEW3DFACTORY_H
#define VIEW3DFACTORY_H

#include "RideMetric.h"
#include <QList>
#include <QColor>
#include <QHash>

class RideItem;
class RideWall;
class UserData;

///////////////////////////////////////////////////////////////////////////////
/// \brief The RideWallFactory class
///        This class generates 3D walls.
///////////////////////////////////////////////////////////////////////////////
class RideWallFactory
{
public:
    // Singleton.
    static RideWallFactory &instance() {
        // Create a new instance if none exist.
        if (!_instance) {
            _instance = new RideWallFactory();
            _instance->computeMethodsMap.insert(DATASERIES, &getSeriesValue);
            _instance->computeMethodsMap.insert(USERDATA, &getUserDataValue);
            _instance->rangeMethodsMap.insert(DATASERIES, &getWallRange);
            _instance->rangeMethodsMap.insert(USERDATA, &getWallRange);
        }
        return *_instance;
    }

    // Getters
    RideWall *getRideWall(const RideFile::seriestype iSeriesType);
    RideWall *getRideWall(UserData *iUserData);
    int getIndexOf(RideWall *);
    int getUserDataIndexOf(RideWall *);

    // Get the total number of 3D walls
    int count() const { return createdSeriesWallsMap.count() + createdUserDataWallsMap.count(); }
    void clearAll();
    void removeUserDataWall(UserData *);
    RideWall* operator[](int i);

    enum eRideWallType { DATASERIES, USERDATA };
    typedef enum eRideWallType RideWallType;
    static const int32_t NO_DATA_PRESENT = -999999;

private:
    // Constructor.
    RideWallFactory() {}

    // Copy constructor.
    RideWallFactory(const RideWallFactory &other);
    RideWallFactory &operator=(const RideWallFactory &other);

    // Specific getters.
    static double getSeriesValue(const double, const double, const int, RideItem*);
    static double getUserDataValue(const double, const double, const int, RideItem*);
    static void getWallRange(QList<double> &ioPointsList, QList<QColor> &ioColorsList, const QColor iColor);

    QList<QPair<RideFile::SeriesType, RideWall *>> createdSeriesWallsMap;
    QList<QPair<UserData *, RideWall *>> createdUserDataWallsMap;

    QHash<RideWallType, double (*)(const double, const double, const int, RideItem*)> computeMethodsMap;
    QHash<RideWallType, void (*)(QList<double> &, QList<QColor> &, const QColor)> rangeMethodsMap;

    static RideWallFactory *_instance;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief The RideWall class.
///        This class is a representation of a 3D wall.
///////////////////////////////////////////////////////////////////////////////
class RideWall : public QObject
{
    Q_OBJECT

friend class RideWallFactory;

public:

    void resetDefault();
    void resetMinMax() { m_validMinMax = false; }
    void resetWallData();
    void buildZoneString();

    // Getters
    double getValue(const double iStart, const double iStop, RideItem *iRideItem);
    RideWallFactory::eRideWallType getType() const { return m_type; }
    double getMin() const { return m_min;}
    double getMax() const { return m_max;}
    QList<double> &pointsList() { return m_ptsList; }
    QList<QColor> &colorsList() { return m_colorList; }
    QString getZoneColors() const { return m_rangeColoEx; }
    bool isValid() const { return m_isValid; }
    bool isDefaultColors() const { return m_isDefaultColors; }
    bool isDefaultPoints() const { return m_isDefaultPoints; }
    QString getName() const { return m_name; }

    // Setters
    void setMinMax(const double iValue);
    void setPointsList(const QList<double> iPointsList) { m_isDefaultPoints = false; m_ptsList = iPointsList; }    // Set user defined points.
    void setColorsList(const QList<QColor> iColorsList) { m_isDefaultColors = false; m_colorList = iColorsList; }  // Set user defined colors.

private:
    // Constructor
    RideWall(const RideWallFactory::RideWallType iType,
             void (*rgFct)(QList<double> &iPointsList, QList<QColor> &iColorsList, const QColor iColor),
             double (*fct)(const double, const double, const int, RideItem *),
             QString iName, UserData *iUserData = nullptr);

    // Ptr functions
    double (*valueFct)(const double, const double, const int, RideItem *);
    void (*rangeFct)(QList<double> &pointsList, QList<QColor> &colorsList, const QColor);

    // Getters
    void getRange(QList<double> &ioPointsList, QList<QColor> &ioColorsList);

    void autosetZoneSettings();
    bool validateZones();

    // Members data
    RideItem *m_rideItem = nullptr;
    UserData *m_userData = nullptr;
    QList<double> m_ptsList;
    QList<QColor> m_colorList;
    QString m_rangeColoEx;
    QString m_name;             // DataSeries symbol or UserData series name.

    double m_min = 0;
    double m_max = 0;
    RideWallFactory::RideWallType m_type = RideWallFactory::DATASERIES;
    bool m_isValid = true;
    bool m_isDefaultColors = true;
    bool m_isDefaultPoints = true;
    bool m_validMinMax = false;

    static const int kNumberOfZones = 5;
};

#endif // VIEW3DFACTORY_H
