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

#include "View3DFactory.h"

#include "UserData.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "Colors.h"

RideWallFactory *RideWallFactory::_instance = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getRideWall
///        This method creates a new 3D wall for a specific series type.
///
/// \param[in] iSeriesType  Series type requested.
///
/// \return A pointer to a 3D wall.
///////////////////////////////////////////////////////////////////////////////
RideWall *RideWallFactory::getRideWall(const RideFile::seriestype iSeriesType)
{
    // Check for existing 3D wall.
    RideWall *wRideWall = nullptr;
    for(auto &wRwItr : createdSeriesWallsMap)
    {
        // Return 3D wall.
        if(wRwItr.first == iSeriesType)
        {
            wRideWall = wRwItr.second;
            return wRideWall;
        }
    }

    // Series which symbol string are defined.
    // index, secs, cad, cadd, hr, hrd, km, kph, kphd, nm, nmd, watts, wattsd,
    // alt, lon, lat, IsoPower, headwind, slope, temp, lrbalance, lte, rte,
    // lps, rps, smo2, thb, rvert, rcad, rcontact, lpco, rpco, lppb, rppb,
    // lppe, rppe, lpppb, rpppb, lpppe, rpppe

    // Create 3D wall only for those series types.
    switch (iSeriesType)
    {
    case RideFile::kph:
    case RideFile::watts:
    case RideFile::IsoPower:
    case RideFile::hr:
    case RideFile::cad:
    case RideFile::rcad:
    case RideFile::nm:
    case RideFile::headwind:
    case RideFile::temp:
    case RideFile::smo2:
    case RideFile::thb:
    case RideFile::rcontact:
    case RideFile::rvert:
    case RideFile::none:    // None is an empty wall.
    {
        wRideWall = new RideWall(DATASERIES, rangeMethodsMap[DATASERIES], computeMethodsMap[DATASERIES], RideFile::symbolForSeries(iSeriesType));
        createdSeriesWallsMap << QPair<RideFile::SeriesType, RideWall *>(iSeriesType, wRideWall);
        break;
    }
    default:
        wRideWall = nullptr;
        break;
    }

    return wRideWall;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getRideWall
///        This method creates a new 3D wall for user defined.
///
/// \param[i] iUserData User defined data formula.
///
/// \return A pointer to a 3D wall.
///////////////////////////////////////////////////////////////////////////////
RideWall *RideWallFactory::getRideWall(UserData *iUserData)
{
    // Check for existing 3D wall.
    RideWall *wRideWall = nullptr;
    for(auto &wRwItr : createdUserDataWallsMap)
    {
        // Return 3D wall.
        if(wRwItr.first == iUserData)
        {
            wRideWall = wRwItr.second;
            return wRideWall;
        }
    }

    wRideWall = new RideWall(USERDATA, rangeMethodsMap[USERDATA], computeMethodsMap[USERDATA], iUserData->name, iUserData);
    createdUserDataWallsMap << QPair<UserData *, RideWall *>(iUserData, wRideWall);
    return wRideWall;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getIndexOf
///        This method searches for the index of the specified 3D wall relative
///        to the total number of walls created.
///
/// \return The index of the 3D wall.
///////////////////////////////////////////////////////////////////////////////
int RideWallFactory::getIndexOf(RideWall *iRideWall)
{
    int wReturning = -1;

    // Data series.
    if (iRideWall->getType() == DATASERIES) {
        for (auto &wRwItr : createdSeriesWallsMap)
        {
            if (wRwItr.second == iRideWall)
            {
                wReturning = createdSeriesWallsMap.indexOf(wRwItr);
                break;
            }
        }
    }
    // User data.
    else {
        // Find the index of the user data 3D wall.
        int wUserDataIndex = getUserDataIndexOf(iRideWall);

        // Get the factory index of the wall.
        if (wUserDataIndex >= 0)
            wReturning = createdSeriesWallsMap.count() + wUserDataIndex;
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getUserDataIndexOf
///        This method searches for the index of the specified User Data 3D
///        wall.
///
/// \return The index of the 3D wall.
///////////////////////////////////////////////////////////////////////////////
int RideWallFactory::getUserDataIndexOf(RideWall *iRideWall)
{
    int wReturning = -1;

    if (iRideWall->getType() == USERDATA)
    {
        for (auto &wRwItr : createdUserDataWallsMap)
        {
            if (wRwItr.second == iRideWall)
            {
                wReturning = createdUserDataWallsMap.indexOf(wRwItr);
                break;
            }
        }
    }
    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::clearAll
///        This method clear all created 3D wall.
///////////////////////////////////////////////////////////////////////////////
void RideWallFactory::clearAll()
{
    // Delete series 3D walls.
    for(auto &wRwItr : createdSeriesWallsMap) {
        delete wRwItr.second;
    }
    createdSeriesWallsMap.clear();

    // Delete User Data 3D walls.
    for(auto &wRwItr : createdUserDataWallsMap) {
        delete wRwItr.second;
    }
    createdUserDataWallsMap.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::removeUserDataWall
///        This method remove a User Data 3D wall since UserData is dynamic.
///
/// \param[in] iUserData    User defined data formula.
///////////////////////////////////////////////////////////////////////////////
void RideWallFactory::removeUserDataWall(UserData *iUserData)
{
    qDebug() << Q_FUNC_INFO;

    if (iUserData != nullptr)
    {
        qDebug() << "UserData pointer valid.";
        int wIndexToRemove = -1;
        for (auto &wRwItr : createdUserDataWallsMap)
        {
            // Compare UserData pointers.
            if (wRwItr.first == iUserData)
            {
                // Get the index of the wall mapping.
                wIndexToRemove = createdUserDataWallsMap.indexOf(wRwItr);

                // Delete 3D wall.
                delete wRwItr.second;
                wRwItr.second = nullptr;
                break;
            }
        }

        // Remove from user data map.
        if (wIndexToRemove >= 0)
            createdUserDataWallsMap.removeAt(wIndexToRemove);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::operator []
///        Operator [] overload.
///
/// \param[in] i    3D wall index.
///
/// \return A pointer to a 3D wall.
///////////////////////////////////////////////////////////////////////////////
RideWall *RideWallFactory::operator[](int i)
{
    // Return UserData 3D wall for indexes greater than the number of series 3D walls.
    if (i >= createdSeriesWallsMap.count())
    {
        return createdUserDataWallsMap[i - createdSeriesWallsMap.count()].second;
    }
    // Return series 3D wall.
    else {
        return createdSeriesWallsMap[i].second;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getSeriesValue
///        This method calculates an average value of a series for specific
///        period of time.
///
/// \param[in] iStart       Start time in seconds.
/// \param[in] iStop        Stop time in seconds.
/// \param[in] iSeriesType  Series type.
/// \param[in] iRideItem    Pointer to the current RideItem.
///
/// \return The average value of the metric.
///////////////////////////////////////////////////////////////////////////////
double RideWallFactory::getSeriesValue(const double iStart , const double iStop , const int iSeriesType, RideItem *iRideItem)
{
    double wReturning = static_cast<double>(NO_DATA_PRESENT);
    double wFinalValue = 0.0;

    if (iRideItem && iRideItem->ride())
    {
        RideFile *wRide = iRideItem->ride();

        // Create a Specification for iterator
        double startKM = 0.0;
        double stopKM = 0.0;
        int displaySequence = 0;
        QColor color;
        IntervalItem intervalItem(iRideItem, "3D view spec", iStart, iStop, startKM, stopKM, displaySequence, color,
                                  false, RideFileInterval::IntervalType::ALL);
        Specification spec(&intervalItem, wRide->recIntSecs());

        // Check if no data present.
        if(!wRide->isDataPresent(static_cast<RideFile::seriestype>(iSeriesType))) {
            return wReturning;
        }

        // NOW USE OUR ITERATOR.
        int wCount = 0;
        RideFileIterator it(wRide, spec);
        while (it.hasNext()) {
            RideFilePoint *point = it.next();
            wFinalValue += point->value(static_cast<RideFile::seriestype>(iSeriesType));
            wCount++;
        }

        wReturning = (wCount > 0) ? (wFinalValue / wCount) : 0.0;
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getUserDataValue
///        This method calculates an average value of a User Data formula for
///        specific period of time.
///
/// \param[in] iStart       Start time in seconds.
/// \param[in] iStop        Stop time in seconds.
/// \param[in] iIndex       Index from user data 3D wall list.
/// \param[in] iRideItem    Current RideItem.
///
/// \return The average value of the metric.
///////////////////////////////////////////////////////////////////////////////
double RideWallFactory::getUserDataValue(const double iStart, const double iStop, const int iIndex, RideItem *iRideItem)
{
    double wReturning = static_cast<double>(NO_DATA_PRESENT);
    bool wIndexValid = (iIndex >= 0) && (iIndex < instance().createdUserDataWallsMap.count());

    if (wIndexValid && iRideItem && iRideItem->ride())
    {
        int wStart = iRideItem->ride()->timeIndex(iStart);
        int wStop = iRideItem->ride()->timeIndex(iStop);
        UserData *wUserData = instance().createdUserDataWallsMap[iIndex].first;

        double wFinalValue = 0.0;

        // Sanity check
        bool wDataValid = false;
        for (auto &wDataItr : wUserData->vector)
            wDataValid |= (std::abs(wDataItr) > 0);

        if ((wUserData == nullptr) || (wDataValid == false)) {
            return wReturning;
        }

        if (wStop >= wUserData->vector.size())
            wStop = wUserData->vector.size() - 1;

        int count = 0;
        for (int i = wStart; i < wStop; i++)
        {
            wFinalValue += wUserData->vector[i];
            count++;
        }

        wReturning = (count > 0) ? (wFinalValue / count) : 0.0;
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWallFactory::getWallRange
///        This method sets the default zones points and colors.
///
/// \param[in/out] ioPointsList     Zones limit points list.
/// \param[in/out] ioColorsList     Zones color list.
/// \param[in] iBaseColor           Base color of the metric.
///////////////////////////////////////////////////////////////////////////////
void RideWallFactory::getWallRange(QList<double> &ioPointsList, QList<QColor> &ioColorsList, const QColor iBaseColor)
{
    // Load default points as percentages.
    if (ioPointsList.empty())
    {
        // Not really used since autoset will clear all. Keep it in case there is an issue.
        ioPointsList << 0.20 << 0.40 << 0.60 << 0.80;
    }

    // Load default zones color.
    if (ioColorsList.empty())
    {
        // Check validity of the base color. Black color is prohibited since it is the color of invalid data.
        // White color is also prohibited but for logic issues.
        if ((iBaseColor == QColor()) || (iBaseColor.name() == "#000000") || (iBaseColor.name() == "#ffffff"))
        {
            ioColorsList << QColor("#0000ff") << QColor("#00ff00") << QColor("#ffff00") << QColor("#ff5500") << QColor("#ff0000");
        }
        // Set zones color based on the metric from ligther to darker.
        else
        {
            QColor wFirstColor = iBaseColor.lighter(300);

            QColor wColor2 = iBaseColor.lighter(150);
            if (wColor2.name() == "#ffffff")
                wColor2 = wFirstColor.darker(105);

            QColor wLastColor = iBaseColor.darker(300);

            // Black color is prohibited. It is the color of invalid data.
            if (wLastColor.name() == "#000000")
                wLastColor = wLastColor.lighter(125);

            QColor wColor4 = iBaseColor.darker(150);

            // Black color is prohibited. It is the color of invalid data.
            if (wColor4.name() == "#000000")
            {
                wColor4 = wLastColor.lighter(105);
            }

            ioColorsList << wFirstColor << wColor2 << iBaseColor << wColor4 << wLastColor;
        }
        //    blue      #0000ff
        //    green     #00ff00
        //    yellow    #ffff00
        //    orange    #ff5500
        //    red       #ff0000
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::RideWall
///        Constructor.
///
/// \param[in] iType        RideWall type. (DataSeries or UserData)
/// \fn        rgFct        Function pointer for getting metric value.
/// \fn        fct          Function pointer for setting and getting zones color.
/// \param[in] iName        Name or symbol of the metric.
/// \param[in] iUserData    UserData pointer.
///////////////////////////////////////////////////////////////////////////////
RideWall::RideWall(const RideWallFactory::RideWallType iType,
                   void (*rgFct)(QList<double> &pointsList, QList<QColor> &colorsList, const QColor),
                   double (*fct)(double, double, const int, RideItem*),
                   QString iName, UserData *iUserData) :
    valueFct(fct), rangeFct(rgFct), m_userData(iUserData), m_name(iName), m_type(iType)
{
    // Load default colors and points values.
    getRange(m_ptsList, m_colorList);

    bool wTypeValid = ((m_type == RideWallFactory::DATASERIES) || (m_type == RideWallFactory::USERDATA));
    bool wNoneSeries = (m_type == RideWallFactory::DATASERIES) && iName.isEmpty();

    m_isValid = wTypeValid && validateZones() && (wNoneSeries == false);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::getRange
///        This method loads the default zones colors and limits points
///        depending on the 3D wall type.
///
/// \param[in/out] ioPointsList     Zones limits points list.
/// \param[in/out] ioColorsList     Zones color list.
///////////////////////////////////////////////////////////////////////////////
void RideWall::getRange(QList<double> &ioPointsList, QList<QColor> &ioColorsList)
{
    // Get the metric color.
    QColor wBaseColor;
    if (m_type == RideWallFactory::DATASERIES)
    {
        wBaseColor = RideFile::colorFor(RideFile::seriesForSymbol(m_name));
    }
    else if (m_userData)
    {
        wBaseColor = m_userData->color;
    }

    // Loads default colors and points.
    rangeFct(ioPointsList, ioColorsList, wBaseColor);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::autosetZoneSettings
///        This method calculates zones limits depending on the current ride
///        minimum and maximum values. It also gets the default zone colors.
///////////////////////////////////////////////////////////////////////////////
void RideWall::autosetZoneSettings()
{
    qDebug() << Q_FUNC_INFO;

    // Update default points.
    if (m_isDefaultPoints)
    {
        if (m_validMinMax)
        {
            int wNbPoints = m_ptsList.count() + 1;
            m_ptsList.clear();

            // Calculates zones limits.
            double wDelta = (m_max - m_min) / wNbPoints;
            for(int i = 1; i < wNbPoints; ++i)
            {
                m_ptsList << m_min + i * wDelta;
            }
        }
        else
        {
            m_isValid = false;
        }
    }

    // Get default colors.
    if (m_isDefaultColors)
    {
        // Clear colors list and load new default zones colors.
        m_colorList.clear();
        getRange(m_ptsList, m_colorList);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::validateZones
///        This method validates the 3D wall zones definitions. It checks for
///        the right number of zones and verify the colors validity.
///
/// \return The validity of zones.
///////////////////////////////////////////////////////////////////////////////
bool RideWall::validateZones()
{
    bool wZonesValid = !m_ptsList.isEmpty() && !m_colorList.isEmpty() &&
                       (m_colorList.count() == (m_ptsList.count() + 1)) &&
                       m_colorList.count() == kNumberOfZones;

    // Validate colors.
    if (wZonesValid)
    {
        for(auto &wColorItr : m_colorList)
            wZonesValid &= wColorItr.isValid();
    }

    return wZonesValid;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::resetDefault
///        This method resets default values.
///////////////////////////////////////////////////////////////////////////////
void RideWall::resetDefault()
{
    // Set default flags.
    m_isDefaultColors = true;
    m_isDefaultPoints = true;

    // Rebuild zone string.
    buildZoneString();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::resetWallData
///        This method resets the 3D wall validity and its minimum and maximum
///        values. An empty wall (Standard dataseries "none") is set as
///        invalid.
///////////////////////////////////////////////////////////////////////////////
void RideWall::resetWallData()
{
    m_isValid = !((m_type == RideWallFactory::DATASERIES) && m_name.isEmpty());
    m_validMinMax = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::buildZoneString
///        This method builds up a zones string that will part of the
///        javascript command to generate the 3D wall.
///
/// \param[in] iInvalid     Indicates that the metric data is invalid.
///////////////////////////////////////////////////////////////////////////////
void RideWall::buildZoneString()
{
    qDebug() << Q_FUNC_INFO << "Wall name" << m_name;

    // Set default zones.
    autosetZoneSettings();

    // Build zones string if zones are correctly defined.
    if (validateZones())
    {
        m_rangeColoEx = "zones = {intervals : [";

        // Add the limits points to the string.
        for(auto &e : m_ptsList)
        {
            m_rangeColoEx += QString::number(e) + ",";
        }

        m_rangeColoEx.chop(1);
        m_rangeColoEx += "], colors : [";

        // Add the zones colors to the string.
        for(auto &wColorItr : m_colorList)
        {
            QString wColorName = (!m_isValid) ? QColor("#000000").name() : wColorItr.name();
            m_rangeColoEx += "'" + wColorName + "', ";
        }

        m_rangeColoEx.chop(2);
        m_rangeColoEx += "]}";
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::getValue
///        This method determines which function to use to calculate the
///        average value of the 3D wall for specific period of time.
///
/// \param[in] iStart       Start time in seconds.
/// \param[in] iStop        Stop time in seconds.
/// \param[in] iRideItem    Current RideItem.
///
/// \return The average value for a specific period of time.
///////////////////////////////////////////////////////////////////////////////
double RideWall::getValue(const double iStart, const double iStop, RideItem *iRideItem)
{
    int wIndex = 0;

    // For data series, the index is the series type.
    if (m_type == RideWallFactory::DATASERIES)
    {
        // Get the series type with its symbol name.
        wIndex = iRideItem->ride()->seriesForSymbol(m_name);
    }
    // For user data, the index is the factory user data index.
    else
    {
        wIndex = RideWallFactory::instance().getUserDataIndexOf(this);
    }

    // Return the average value.
    double wSectionValue = valueFct(iStart, iStop, wIndex, iRideItem);

    // Check if data is invalid.
    if (static_cast<int32_t>(wSectionValue) == RideWallFactory::NO_DATA_PRESENT)
    {
        wSectionValue = 0;
        m_isValid = false;
    }

    setMinMax(wSectionValue);

    return wSectionValue;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief RideWall::setMinMax
///        This method is called to set the metric minimum and maximum values.
///
/// \param[in] value
///////////////////////////////////////////////////////////////////////////////
void RideWall::setMinMax(const double iValue)
{
    // No maximum nor minimum set.
    if(!m_validMinMax)
    {
        // Set min and max with the input.
        m_min = m_max = iValue;

        // We have a valid min and/or maxé
        m_validMinMax = true;
    }
    else
    {
        // Determine maximum and minimum.
        if (iValue < m_min)
            m_min = iValue;

        if (iValue > m_max)
            m_max = iValue;
    }
}
