/*
 * Copyright (c) 2017 NotioKonect
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

#include "NotioKonect.h"
#include "Athlete.h"
#include "RideCache.h"
#include "Settings.h"
#include "Tab.h"
#include "JsonRideFile.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QFileDialog>
#include <QHttpMultiPart>

#include <QProgressDialog>

#include <cstdio>

#ifndef GC_VERSION
#define GC_VERSION "(developer build)"
#endif

namespace {
    static const QString  kNotioKonectApiBaseAddr = "https://cloud.notiokonect.com";
    static const QString  kNotioKonectApiActivities = "/api/v1/activities";
    static const QString  kNotioKonectApiMerge = "/api/v1/activities/merge";
    static const QString  kNotioKonectApiNotio = "/api/v1/activities/notio/";
    static const QString  kNotioKonectApiFit = "/api/v1/activities/fit/";
    static const QString  kNotioKonectApiGcjson = "/api/v1/activities/gcjson/";
    static const QString  kNotioKonectApiTrainees = "/api/v1/trainees";
}

struct NotioKonect::FileInfo {
    QString name;
    QString id;  // This is the unique identifier.
    QString parent; // id of the parent.
    QString download_url;

    QDateTime modified;
    int size;
    bool is_dir;

    // This is a map of names => FileInfos for quick searching of children.
    std::map<QString, QSharedPointer<FileInfo> > children;
};

NotioKonect::NotioKonect(Context *context) : CloudService(context), loadType(A18_UNKNOWN), replySuccess_(false), root_(NULL), progressDialog_(NULL)
{
    if (context) {
        nam_ = new QNetworkAccessManager(this);
        connect(nam_, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
        connect(this, SIGNAL(msgInfoBox(QString, QString)), reinterpret_cast<QObject*>(const_cast<MainWindow*>(context->mainWindow)),
                SLOT(messageInfoBox(QString, QString)), Qt::QueuedConnection);
    }
    root_ = NULL;

    // how is data uploaded and downloaded?
    uploadCompression = none;
    downloadCompression = none;

    useMetric = true; // distance and duration metadata

    //printd("NotioKonect::NotioKonect");

    // config
    settings.insert(OAuthToken, GC_NOTIOKONECT_ACCESS_TOKEN);
    //settings.insert(Folder, GC_NOTIOKONECT_FOLDER);
    settings.insert(Local1, GC_NOTIOKONECT_REFRESH_TOKEN); // derived during config, no user action
    settings.insert(Local2, GC_NOTIOKONECT_LAST_ACCESS_TOKEN_REFRESH); // derived during config, no user action

    // We don't want to display the Coached atheles selection page when adding cloud account.
    // Disabled by default, but these settings are added later if needed. (see selectAthlete())
    //settings.insert(AthleteID, GC_NOTIOKONECT_ATHLETE_ID);
    //settings.insert(Local3, GC_NOTIOKONECT_ATHLETE_NAME);
}


NotioKonect::~NotioKonect()
{
    //if (context) delete nam_; // has this a sparent
}

void NotioKonect::onSslErrors(QNetworkReply*reply, const QList<QSslError> &)
{
    qDebug() << Q_FUNC_INFO;
    reply->ignoreSslErrors();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::open
///        This method is call upon opening and connecting to cloud service. It
///        verifies the credentials. It also requests the athletes' list and
///        selects the current athlete from the settings else select the Notio
///        account owner.
///
/// \param[out] errors Error message.
/// \return Status
///////////////////////////////////////////////////////////////////////////////////
bool NotioKonect::open(QStringList &errors) {
    bool wReturning = true;
    // Check credentials.
    if ( validateCredentials(errors) != SUCCESS) {
        errors << tr("You must authorise with Notio Konect first. Add a Notio cloud account.");
        invalidateCredentials();
        return false;
    }

    // Check for access token.
    QString wToken = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_ACCESS_TOKEN, "").toString().toUtf8());
    if (wToken.isEmpty())
    {
        errors << tr("No token access after connection check! Verify your Notio Konect credentials.");
        return false;
    }

    // Request to the cloud to get the list of athletes.
    QString arg = "";
    QNetworkRequest request = MakeRequestWithURL(kNotioKonectApiTrainees, wToken, arg);

    QNetworkReply *reply = nam_->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    loop.exec();

    // did we get a good response ?
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray r = reply->readAll();        
        qDebug() << QString("Request error: %1[%2]").arg(QString::number(reply->error()), reply->errorString());
        qDebug() << "HTTP response code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
        qDebug() << "Request response:" << r.data();

        errors << tr("Request(from-to) reply error: ") + reply->errorString();
        return false;
    }

    // Parsing of the JSON files received.
    QByteArray r = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error == QJsonParseError::NoError)
    {
        // Find the current athlete from the requested list.
        QJsonArray resultArray = document.object()["result"].toArray();
        if (resultArray.count() > 0)
        {
            // Get local user ID.
            QString wUserId = appsettings->cvalue(context->athlete->cyclist, GC_NOTIOKONECT_ATHLETE_ID, "").toString();
            CloudServiceAthlete wAccountOwner, wSelectAthlete;
            bool wUserFound = false;

            // Scan through the list.
            for (int i = 0; i < resultArray.count(); i++)
            {
                QString wId = QString("%1").arg(resultArray[i].toObject()["id"].toString());
                QString wFirstName = resultArray[i].toObject()["firstName"].toString();
                QString wLastName = resultArray[i].toObject()["lastName"].toString();
                QString wFullName = wFirstName + ((wFirstName.isEmpty() || wLastName.isEmpty()) ? "" : " ") + wLastName;

                // Check that the local ID matches. Select first athlete if it is a newly added account.
                if (wUserId.isEmpty() || (wId == wUserId))
                {
                    wUserFound = true;
                    wSelectAthlete.id = wId;
                    wSelectAthlete.name = wFullName;
                    wSelectAthlete.desc = "Athlete";
                    break;
                }

                // Get first athlete (account owner) information.
                if (i == 0)
                {
                    wAccountOwner.id = wId;
                    wAccountOwner.name = wFullName;
                    wAccountOwner.desc = "Athlete";
                }
            }

            // The current user ID cannot be found. Select cloud account owner.
            if (!wUserFound)
            {
                QString wUserName = appsettings->cvalue(context->athlete->cyclist, GC_NOTIOKONECT_ATHLETE_NAME, "").toString();
                QString wMessage = QString(tr("The current athlete <b>%1</b> is not available for this cloud account.")).arg(wUserName);
                wMessage.append(tr(" Selecting the cloud account owner instead."));
                MessageInfo(wMessage);
                wSelectAthlete = wAccountOwner;
            }

            // Select current athlete.
            selectAthlete(wSelectAthlete);
        }
        // Request result empty.
        else {
            errors << tr("Error: no athlete retrieved from the cloud. Please contact Notio Konect support.");
            wReturning = false;
        }
    }
    // Cannot parse JSON file.
    else {
        errors << tr("Error parsing JSON data when populating athletes' list. Please contact Notio Konect support.");
        wReturning = false;
    }

    return wReturning;
}

bool NotioKonect::close()
{
    // nothing to do for now
    return true;
}

// home dire
QString NotioKonect::home()
{
    return getSetting(GC_NOTIOKONECT_FOLDER, "").toString();
}

QNetworkRequest NotioKonect::MakeRequestWithURL(const QString& url, const QString& token, const QString& args)
{
    QString request_url;
    request_url =  kNotioKonectApiBaseAddr + url + args;

    QNetworkRequest request(request_url);
    request.setRawHeader("x-access-token", token.toLatin1());
    return request;
}

bool NotioKonect::createFolder(QString path)
{
    Q_UNUSED(path)
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::listAthletes
///        This method requests the athletes' list from Notio cloud account to see
///        if there are more than one athlete.
///        The first athlete is always the owner. The following athletes, if any,
///        are then athletes coached by the first in the list (also called coach).
///
/// \return The athletes' list.
///////////////////////////////////////////////////////////////////////////////////
QList<CloudServiceAthlete> NotioKonect::listAthletes()
{
    QList<CloudServiceAthlete> wReturning;

    QString token = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_ACCESS_TOKEN, "").toString().toUtf8());
    if (token == "") {
        return QList<CloudServiceAthlete> ();
    }

    // Request to the cloud to get the list of athletes.
    QString arg = "";
    QNetworkRequest request = MakeRequestWithURL(kNotioKonectApiTrainees, token, arg);

    QNetworkReply *reply = nam_->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    loop.exec();

    // did we get a good response ?
    if (reply->error() != QNetworkReply::NoError)
    {
        QByteArray r = reply->readAll();
        qDebug() << QString("Request error: %1[%2]").arg(QString::number(reply->error()), reply->errorString());
        qDebug() << "HTTP response code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
        qDebug() << "Request response:" << r.data();
        qDebug() << tr("Request(from-to) reply error: ") + reply->errorString();

        return QList<CloudServiceAthlete> ();
    }

    // Parsing of the JSON files received.
    QByteArray r = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error == QJsonParseError::NoError)
    {
        QJsonArray resultArray = document.object()["result"].toArray();
        if (resultArray.count() > 0)
        {
            // Populates list.
            for (int i=0;i<resultArray.count();i++)
            {
                CloudServiceAthlete add;
                add.id = QString("%1").arg(resultArray[i].toObject()["id"].toString());
                QString wFirstName = resultArray[i].toObject()["firstName"].toString();
                QString wLastName = resultArray[i].toObject()["lastName"].toString();
                QString wFullName = wFirstName + ((wFirstName.isEmpty() || wLastName.isEmpty()) ? "" : " ") + wLastName;
                add.name = wFullName;
                add.desc = "Athlete";
                wReturning << add;
            }
        }
    }
    else {
        qDebug() << tr("Error parsing JSON data when populating athletes' list.");
        return QList<CloudServiceAthlete> ();
    }

    // More than one athlete.
    if (wReturning.count() > 1)
    {
        // First athlete is always a coach.
        wReturning[0].name.prepend(tr("Coach (")).append(")");
        wReturning[0].desc = tr("Athlete or group of athletes (team)");
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::selectAthlete
///        This method save the selected athlete in the athlete private
///        configuration file.
///
/// \param[in] athlete
/// \return Status.
///////////////////////////////////////////////////////////////////////////////////
bool NotioKonect::selectAthlete(CloudServiceAthlete athlete)
{
    // Add athlete's ID profile setting.
    if (settings.find(AthleteID) == settings.end())
    {
        settings.insert(AthleteID, GC_NOTIOKONECT_ATHLETE_ID);
    }

    // Add athlete's name profile setting.
    if (settings.find(Local3) == settings.end())
    {
        settings.insert(Local3, GC_NOTIOKONECT_ATHLETE_NAME);
    }

    // Save athlete ID and name.
    setSetting(GC_NOTIOKONECT_ATHLETE_ID, athlete.id);
    setSetting(GC_NOTIOKONECT_ATHLETE_NAME, athlete.name);
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

QList<CloudServiceEntry*> NotioKonect::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    qDebug() << Q_FUNC_INFO << from.toString("yy-MM-dd") << to.toString("yy-MM-dd");

    QList<CloudServiceEntry*> returning;

    // Trim some / if necssary.
    while (*path.end() == '/' && path.size() > 1) {
        path = path.remove(path.size() - 1, 1);
    }
    QString token = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_ACCESS_TOKEN, "").toString().toUtf8());
    if (token == "")
    {
        errors << tr("No token access after connection check!!!");
        return returning;
    }

    QString arg = "?from=" + from.date().toString("yyyy-MM-dd") + "&to=" + to.date().toString("yyyy-MM-dd") + actForUserID(APPEND_ARG);
    QNetworkRequest request = MakeRequestWithURL(kNotioKonectApiActivities, token, arg);

    QNetworkReply *reply = nam_->get(request);

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyFileDiagUpload()));

    // Reading activities list ...
    loop.exec();

    // Analyzing activities list ...
    QApplication::processEvents();

    // did we get a good response ?   
    if (reply->error() != QNetworkReply::NoError)
    {
        QByteArray r = reply->readAll();
        qDebug() << QString("Request error: %1[%2]").arg(QString::number(reply->error()), reply->errorString());
        qDebug() << "HTTP response code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
        qDebug() << "Request response:" << r.data();
        MessageInfo("Request(from-to) reply error: " + reply->errorString());

		return returning;
    }

	QByteArray r = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // If there's an error just give up.
    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << "json parse error: ....";
        qDebug() << QString("reply: %1").arg(r.data());
        errors << tr("Json Parsing error");
        return returning;
    }

    if (document.object()["success"].toBool() == false || document.object()["result"].isNull()
            || document.object()["result"].toObject()["activities"].isNull())
    {
        qDebug() << "Request return false success";
        qDebug() << QString("reply: %1").arg(r.data());
        errors << tr("Request returned with errors");
        return returning;
    }

    QJsonArray resultArray = document.object()["result"].toObject()["activities"].toArray();

    if (resultArray.count() == 0)
    {
        qDebug() << "No files to download";
        qDebug() << QString("reply: %1").arg(r.data());
        return returning;
    }

    foreach (const QJsonValue & elem, resultArray)
    {        
        QJsonObject obj = ReadJsonElement(elem.toArray());

        CloudServiceEntry *add = newCloudServiceEntry();

        // file details
        QString wSuffix = obj["data"].toString();
        add->type = wSuffix;
        if (wSuffix != "fit")
        {
            // Notio (either merge or not) and gcjson files.
            wSuffix = "json";
        }

        // Notio Konect's Label is the title of the ride.
        add->label = obj["title"].toString();
        add->sport = obj["sport"].toString();

        // Put the cloud ID.
        add->id = obj["id"].toString();

        add->isDir = false;
        add->size = 0;
        add->modified = QDateTime::fromString(obj["lastModification"].toString(), Qt::ISODate);
        add->distance = obj["distance"].toDouble();
        add->duration = static_cast<long>(obj["duration"].toDouble());

        add->devices = obj["deviceType"].toString();

        add->name = QDateTime::fromString(obj["start"].toString(), Qt::ISODate).toString("yyyy_MM_dd_hh_mm_ss") + "." + wSuffix;

        // only our own name is a reliable key
        m_replyActivity.insert(add->name, obj);

        returning << add;
    }

    return returning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::ReadJsonElement
///        This method parses a JSON element containing a Notio Konect activity
///        metadata. When an activity data is the result of a merge between FIT
///        and Notio files, it concatenates the data for the cloud IDs and the
///        device types of each source.
///
/// \param[in] iElement JSON element containing metadata.
/// \return A JSON object.
///////////////////////////////////////////////////////////////////////////////////
QJsonObject NotioKonect::ReadJsonElement(const QJsonArray & iElement)
{
    QJsonObject wReturning;
    static const int NB_FILES_MERGED = 2;

    // Check for an array representing a merged file.
    if (iElement.count() > 1)
    {
        // Find the Notio file metadata. The size of the array will always be 2.
        for (int i = 0; (i < iElement.size()) && (iElement.size() == NB_FILES_MERGED); i++)
        {
            QJsonObject wFileMeta = iElement.at(i).toObject();

            // Return object if GCJSON is present in metadata.
            if (wFileMeta["data"].toString() == "gcjson")
            {
                wReturning = wFileMeta;
            }
            // We consider the data from the Notio part.
            else if (wFileMeta["data"].toString() == "notio")
            {
                int wFitDataIndex = NB_FILES_MERGED - i - 1;
                // Get the metadata structure and then modify the ID and the device type fields.
                wReturning = wFileMeta;

                // Make a string composed of both IDs seperated by a semicolon.
                wReturning["id"] = QString(wFileMeta["id"].toString() + ";" + iElement.at(wFitDataIndex).toObject()["id"].toString());

                // Make a string composed of both device type.
                wReturning["deviceType"] = QString(wFileMeta["deviceType"].toString() + "; " + iElement.at(wFitDataIndex).toObject()["deviceType"].toString());

                // Put the last modification time of the more recent element.
                QDateTime wNotioLastModif = QDateTime::fromString(wFileMeta["lastModification"].toString(), Qt::ISODate);
                QString wFitLastModifStr = iElement.at(wFitDataIndex).toObject()["lastModification"].toString();

                if (QDateTime::fromString(wFitLastModifStr, Qt::ISODate) > wNotioLastModif)
                    wReturning["lastModification"] = wFitLastModifStr;

                break;
            }
        }
    }
    // The metadata represent a single file.
    else
         wReturning = iElement.first().toObject();

    return wReturning;
}

QList<CloudServiceEntry*> NotioKonect::readdir(QString path, QStringList &errors)
{
    Q_UNUSED(path)
    Q_UNUSED(errors);

    QMessageBox Msgbox;
    Msgbox.setText(tr("NotioKonect::readdir from path without from to dates is not implemented"));
    Msgbox.exec();

    QList<CloudServiceEntry*> returning;
    return returning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::readFile
///        This method constructs the network request to download an activity from
///        the cloud server.
///
/// \param data[out]        Raw data extracted
/// \param remotename[in]   Activity filename.
/// \param iRemoteId[in]    Activity cloud storage ID. (Not used)
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool NotioKonect::readFile(QByteArray *data, QString remotename, QString iRemoteId)
{
    Q_UNUSED(iRemoteId)

    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename,
    // QString message) when done

    // do we have a token?
    QStringList wErrors;
    if (validateCredentials(wErrors) != SUCCESS)
    {
        wErrors << tr("You must authorise with Notio Konect first. Add a Notio cloud account.");
        MessageInfo(wErrors.join("\n"));
        return false;
    }

    QString token = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_ACCESS_TOKEN, "").toString().toUtf8());
    if (token == "")
    {
        MessageInfo("No token access after connection check! Verify your Notio Konect credentials.");
        return false;
    }

    // Iterate over list and find remote name
    QList<CloudServiceEntry*>::iterator it = list_.begin();

    CloudServiceEntry *entryToDownload = nullptr;

    for ( ; it != list_.end(); ++it ) {
        if( (*it)->name == remotename) {
            entryToDownload = (*it);
            break;
        }
    }

    if (entryToDownload == nullptr)
        return false;
    else
    {
        QNetworkRequest request;

        // The download entry contains Notio Konect device data.
        if (entryToDownload->type == "notio")
        {
            QStringList wIds = entryToDownload->id.split(';');

            // Trying to download an activity from a merged between FIT and Notio files.
            if (wIds.size() > 1)
            {
                QString arg = "?notio=" + wIds.at(0) + "&fit=" + wIds.at(1) + actForUserID(APPEND_ARG);
                request = MakeRequestWithURL( kNotioKonectApiMerge, token, arg);
            }
            // Request to download Notio file.
            else
            {
                // direct load request
                QString arg = entryToDownload->id + actForUserID();
                request = MakeRequestWithURL( kNotioKonectApiNotio, token, arg);
            }
        }

        // The download entry is a FIT file.
        else if(entryToDownload->type == "fit")
        {
            QString arg = entryToDownload->id + actForUserID();
            request = MakeRequestWithURL( kNotioKonectApiFit, token, arg);
        }
        // The download entry is a JSON generated by Golden Cheetah or an external source.
        else if (entryToDownload->type == "gcjson")
        {
            QString arg = entryToDownload->id + actForUserID();
            request = MakeRequestWithURL( kNotioKonectApiGcjson, token, arg);
        }
        else
            return false;

        // Get the file.
        QNetworkReply *reply = nam_->get(request);

        // remember the file.
        mapReply(reply, remotename);
        buffers_.insert(reply, data);            

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
        connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
        loop.exec();

        // did we get a good response ?
        if (reply->error() != 0)
        {
            QByteArray r = reply->readAll();
            qDebug() << QString("Request error: %1[%2]").arg(QString::number(reply->error()), reply->errorString());
            qDebug() << "HTTP response code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            qDebug() << "Request response:" << r.data();
            MessageInfo("Request error: " + reply->errorString());
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::prepareResponse
///        This methods prepares the data received for a dowloaded activity. It
///        converts any FIT file received into the JSON format. It also adds the
///        information about the source files' cloud ID from with the activity came
///        from.
///
/// \param iData[in]        Raw data received.
/// \param ioName[in, out]  Name of the activity.
/// \return A buffer containing raw data of the modified activity.
///////////////////////////////////////////////////////////////////////////////////
QByteArray *NotioKonect::prepareResponse(QByteArray *iData, QString &ioName)
{
    // Uncompress and parse the activity.
    QStringList errors;
    RideFile *ride = uncompressRide(iData, ioName, errors);

    if (ride)
    {
        // Get the metadata of the activity.
        QJsonObject activity = m_replyActivity.value(ioName, QJsonObject());

        // Set Source cloud IDs.
        QStringList wIds = activity["id"].toString().split(";", QString::SkipEmptyParts);

        // Metadata for a merged file using both FIT and Notio files will have an array of cloud ID.
        if (wIds.count() > 1)
        {
            // Notio source
            QString wNotioId = QString("%1").arg(wIds.at(0));
            ride->setTag("NOTIO Source Cloud ID", wNotioId);

            // FIT source
            QString wGarminId = QString("%1").arg(wIds.at(1));
            ride->setTag("FIT Source Cloud ID", wGarminId);
        }

        // FIT, Notio or GCJSON file
        else if (activity["data"].isString())
        {
            // Add the Cloud ID source file.
            QString wDataType = activity["data"].toString();
            if (wDataType == "fit")
            {
                ride->setTag("FIT Source Cloud ID", wIds.first());
            }
            else if (wDataType == "notio")
            {
                ride->setTag("NOTIO Source Cloud ID", wIds.first());
            }

            // GCJSON files should already have the information.
        }

        // Set custom device type field.
        ride->setTag("deviceType", activity["deviceType"].toString());

        // Convert JSON to byte array.
        JsonFileReader reader;
        iData->clear();
        iData->append(reader.toByteArray(context, ride, true, true, true, true));

        // Rename the file to have the JSON extension.
        if (QFileInfo(ioName).suffix() != "json") ioName = QFileInfo(ioName).baseName() + ".json";
    }

    return iData;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::writeFile
///        This methods constructs the network request to send an activity onto the
///        cloud server.
///
/// \param data[in]         Raw data of the file.
/// \param remotename[in]   Name of the file to send.
/// \param ride[in]         RideFile object.
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool NotioKonect::writeFile(QByteArray &data, QString remotename, RideFile *ride) {
    QStringList wErrors;
    // do we have a token?
    if( validateCredentials(wErrors) != SUCCESS) {
        wErrors << tr("You must authorise with Notio Konect first. Add a Notio cloud account.");
        MessageInfo(wErrors.join("\n"));
        return false;
    }
    QString wToken = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_ACCESS_TOKEN, "").toString().toUtf8());
    if (wToken == "") {
        MessageInfo("No token access after connection check! Verify your Notio Konect credentials.");
        return false;
    }

    QNetworkRequest wRequest;
    QNetworkReply *wReply = nullptr;

    // Check if the file already exist and need updating.
    bool wUpdateRemoteRide = false;
    QString wRemoteFileId;
    for (auto wCloudEntryItr : list_)
    {
        if (QFileInfo(wCloudEntryItr->name).baseName() == QFileInfo(remotename).baseName())
        {
            // Can update remote file only if it is a Golden Cheetah JSON file.
            if (wCloudEntryItr->type == "gcjson")
            {
                wRemoteFileId = wCloudEntryItr->id;
                wUpdateRemoteRide = true;
                break;
            }
        }
    }

    qDebug() << QString("NotioKonect::writeFile %1 activity %2").arg((wUpdateRemoteRide ? "PUT" : "POST")).arg(remotename.toStdString().c_str());

    QString wStrSize = QString("%1").arg(data.size());

    QString wArg = "/gcjson";

    // If updating, add remote file ID to the request.
    if  (wUpdateRemoteRide)
        wArg += QString("/%1").arg(wRemoteFileId);

    wRequest = MakeRequestWithURL( kNotioKonectApiActivities, wToken, wArg);
    wRequest.setRawHeader("x-access-token", wToken.toLatin1());

    qDebug() << wArg;

    QString wBoundary = QVariant(qrand()).toString() + QVariant(qrand()).toString() + QVariant(qrand()).toString();
    QHttpMultiPart *wMultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    wMultiPart->setBoundary(wBoundary.toLatin1());

    QHttpPart wFileP;
    wFileP.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"" + remotename + "\""));
    wFileP.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    wFileP.setBody(data);

    QHttpPart wDataP;
    QString wMetaData = QString("{\"softwareVer\": \"GCNotioKonect-%1\", \"fileName\": \"%2\", \"sizeInBytes\": %3").arg(QString(GC_VERSION)).arg(remotename).arg(wStrSize);
    wMetaData.append(QString(", \"title\": \"%1\", \"sport\": \"%2\"").arg(ride->getTag("title", "")).arg(ride->getTag("Sport", "")));
    wMetaData.append(QString(", \"source\": [\"%1\", \"%2\"]").arg(ride->getTag("NOTIO Source Cloud ID", "0")).arg(ride->getTag("FIT Source Cloud ID", "0")));
    wMetaData.append(QString(", \"deviceType\": \"%1\"").arg(ride->getTag("deviceType", "")));

    // On behalf of an athlete.
    QString wAthleteID = appsettings->cvalue(context->athlete->cyclist, GC_NOTIOKONECT_ATHLETE_ID, "").toString();
    wMetaData.append((wAthleteID.isEmpty() ? "" : QString(", \"userId\": \"%1\"").arg(wAthleteID)));

    // Start date.
    QDateTime wStartDate = ride->startTime();
    QString wStartDateStr = wStartDate.toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");

    // Calculate end date.
    int wStartSecs = (ride->dataPoints().count()) ? ride->dataPoints().first()->secs : 0;
    int wEndSecs = (ride->dataPoints().count()) ? ride->dataPoints().last()->secs : 0;
    int wRideDuration = (wEndSecs >= wStartSecs) ? (wEndSecs - wStartSecs) : 0;

    QString wEndDate = wStartDate.addSecs(wRideDuration).toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");

    wMetaData.append(QString(", \"start\": \"%1\", \"end\": \"%2\"").arg(wStartDateStr).arg(wEndDate));

    // Extract the last modification date.
    QString wFullPath =  QString(context->athlete->home->activities().absolutePath()) + "/" + remotename;
    QFile wFile(wFullPath);

    QString wLastModified = QFileInfo(wFile).lastModified().toUTC().toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
    wMetaData.append(QString(", \"lastModification\": \"%1\"").arg(wLastModified));

    wMetaData.append("}");

    qDebug() << wMetaData;

    wDataP.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"metaData\""));
    wDataP.setBody(wMetaData.toUtf8());

    wMultiPart->append(wFileP);
    wMultiPart->append(wDataP);

    // Update the current file on the server.
    if (wUpdateRemoteRide)
    {
        wReply = nam_->put(wRequest, wMultiPart );
    }
    // Add a new file on the server.
    else
    {
        wReply = nam_->post(wRequest, wMultiPart );
    }
    wMultiPart->setParent(wReply);

    connect(wReply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));
    connect(wReply, SIGNAL(readyRead()), this, SLOT(readyWrite()));

    mapReply(wReply, remotename);
    return true;
}

void NotioKonect::modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue)
{
    const int indexOfDot = path.indexOf('.');
    const QString dotPropertyName = path.left(indexOfDot);
    const QString dotSubPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

    const int indexOfSquareBracketOpen = path.indexOf('[');
    const int indexOfSquareBracketClose = path.indexOf(']');

    const int arrayIndex = path.mid(indexOfSquareBracketOpen + 1, indexOfSquareBracketClose - indexOfSquareBracketOpen - 1).toInt();

    const QString squareBracketPropertyName = path.left(indexOfSquareBracketOpen);
    const QString squareBracketSubPath = indexOfSquareBracketClose > 0 ? (path.mid(indexOfSquareBracketClose + 1)[0] == '.' ? path.mid(indexOfSquareBracketClose + 2) : path.mid(indexOfSquareBracketClose + 1)) : QString();

    // determine what is first in path. dot or bracket
    bool useDot = true;
    if (indexOfDot >= 0) // there is a dot in path
    {
        if (indexOfSquareBracketOpen >= 0) // there is squarebracket in path
        {
            if (indexOfDot > indexOfSquareBracketOpen)
                useDot = false;
            else
                useDot = true;
        }
        else
            useDot = true;
    }
    else
    {
        if (indexOfSquareBracketOpen >= 0)
            useDot = false;
        else
            useDot = true; // acutally, id doesn't matter, both dot and square bracket don't exist
    }

    QString usedPropertyName = useDot ? dotPropertyName : squareBracketPropertyName;
    QString usedSubPath = useDot ? dotSubPath : squareBracketSubPath;

    QJsonValue subValue;
    if (destValue.isArray())
        subValue = destValue.toArray()[usedPropertyName.toInt()];
    else if (destValue.isObject())
        subValue = destValue.toObject()[usedPropertyName];
    else
        qDebug() << "oh, what should i do now with the following value?! " << destValue;

    if(usedSubPath.isEmpty())
    {
        subValue = newValue;
    }
    else
    {
        if (subValue.isArray())
        {
            QJsonArray arr = subValue.toArray();
            QJsonValue arrEntry = arr[arrayIndex];
            modifyJsonValue(arrEntry,usedSubPath,newValue);
            arr[arrayIndex] = arrEntry;
            subValue = arr;
        }
        else if (subValue.isObject())
            modifyJsonValue(subValue,usedSubPath,newValue);
        else
            subValue = newValue;
    }

    if (destValue.isArray())
    {
        QJsonArray arr = destValue.toArray();
        arr[arrayIndex] = subValue;
        destValue = arr;
    }
    else if (destValue.isObject())
    {
        QJsonObject obj = destValue.toObject();
        obj[usedPropertyName] = subValue;
        destValue = obj;
    }
    else
        destValue = newValue;
}


void NotioKonect::modifyJsonValue(QJsonDocument& doc, const QString& path, const QJsonValue& newValue)
{
    QJsonValue val;
    if (doc.isArray())
        val = doc.array();
    else
        val = doc.object();

    modifyJsonValue(val,path,newValue);

    if (val.isArray())
        doc = QJsonDocument(val.toArray());
    else
        doc = QJsonDocument(val.toObject());
}

void NotioKonect::uploadFileDialog()
{
    QVariant lastDirVar = appsettings->value(this, GC_SETTINGS_LAST_UPLOAD_PATH);
    QString lastDir = (lastDirVar != QVariant()) ? lastDirVar.toString() : QDir::homePath();

    QStringList fileNames;
    QStringList allFormats;
    allFormats << QString("All Supported Formats (*.fit *.json)");
    allFormats << QString("Fit file (*.fit)");
    allFormats << QString("Json file (*.json)");
    allFormats << "All files (*.*)";
    fileNames = QFileDialog::getOpenFileNames( Q_NULLPTR, tr("Upload File to NotioKonect Cloud"), lastDir, allFormats.join(";;"));
    if (!fileNames.isEmpty())
    {
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        appsettings->setValue(GC_SETTINGS_LAST_UPLOAD_PATH, lastDir);

        QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy

        foreach(QString fileToUpload, fileNamesCopy)
            uploadFile(fileToUpload);
    }
}

//to use writeFFile
void NotioKonect::uploadFile(QString fileName)
{
    QStringList wErrors;
    // do we have a token?
    if( validateCredentials(wErrors) != SUCCESS) {
        wErrors << tr("You must authorise with Notio Konect first. Add a Notio cloud account.");
        MessageInfo(wErrors.join("\n"));
        return;
    }
    QString token = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_ACCESS_TOKEN, "").toString().toUtf8());
    if (token == "")
    {
        MessageInfo("No token access after connection check! Verify your Notio Konect credentials.");
        return;
    }

    QProgressDialog * wProgressDialog = getProgressDialog();

    QNetworkRequest request;
    QFileInfo fileInfo(fileName);

    if (fileInfo.fileName().right(3).compare("fit", Qt::CaseInsensitive)  == 0) {

        QFile *file = new QFile(fileName);
        if ( !file->exists() )
        {
            // Should not happen
            MessageInfo("ERROR: File Does not exist");
            return;
        }

        QString strSize = QString::number(file->size());

        file->open(QIODevice::ReadOnly);

        QString arg = actForUserID();
        request = MakeRequestWithURL( kNotioKonectApiFit, token, arg);
        request.setRawHeader("x-access-token", token.toLatin1());

        QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();
        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        multiPart->setBoundary(boundary.toLatin1());

        QHttpPart fileP;
        fileP.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"" + fileInfo.fileName() + "\""));
        fileP.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

        fileP.setBodyDevice(file);
        file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

        QHttpPart dataP;
        QString metaData = QString("{\"source\":\"GoldenCheetah\", \"fileName\":\"") + fileInfo.fileName() + QString("\", \"sizeInBytes\" : ") + strSize + QString("}");
        dataP.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"metaData\""));
        dataP.setBody(metaData.toUtf8());

        multiPart->append(fileP);
        multiPart->append(dataP);

        QNetworkReply *reply = nam_->post(request, multiPart );
        multiPart->setParent(reply);

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(finished()), this, SLOT(uploadFileDiagCompleted()));
        connect(reply, SIGNAL(readyRead()), this, SLOT(readyFileDiagUpload()));

        if (wProgressDialog)
        {
            wProgressDialog->setLabelText("Uploading FIT file ...");
            wProgressDialog->show();
        }

        loop.exec();

        if(replySuccess_ == true) {
            // remember
            mapReply(reply,fileName);
            reply->deleteLater();

            //MessageInfo("File uploaded with success.");
        }
    }
    else {
        MessageInfo("ERROR: Only .fit format is supported.");
    }
    wProgressDialog->close();
}

void NotioKonect::writeFileCompleted()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    QByteArray r = reply->readAll();
    QString wMessage;

    if (reply->error() == QNetworkReply::NoError)
    {
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // If there's an error just give up.
        if (parseError.error != QJsonParseError::NoError)
        {
            qDebug() << "json parse error: ....";
            qDebug() << QString("reply: %1").arg(r.data());
            MessageInfo("Reply from cloud Json Parsing error");
            wMessage = tr("Upload failed") + QString(" ") + QString(r.data());
        }
        else if(document.object()["success"].isBool() == false || document.object()["success"].toBool() == false)
        {
            if (document.object()["Error"].toString().contains("gcjson exist"))
            {
                qDebug() << "Activity already exist.";
                wMessage = tr("Upload failed. Activity already exist.");
            }
            else if (document.object()["Error"].toString().contains("ObjectIDs must be exactly 12 bytes long"))
            {
                wMessage = tr("Upload failed. Ride ID doesn't match with Cloud.");
            }
            else
                wMessage = tr("Upload failed") + QString(" ") + QString(r.data());
        }
        else
            wMessage = tr("Completed.");
    }
    else
    {
        qDebug() << QString("Request error: %1[%2]").arg(QString::number(reply->error()), reply->errorString());
        qDebug() << "HTTP response code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
        qDebug() << "Request response:" << r.data();
        MessageInfo("Request error: " + reply->errorString());
        wMessage = tr("Upload failed") + QString(" ") + QString(r.data());
    }

    notifyWriteComplete(replyName(static_cast<QNetworkReply*>(QObject::sender())), wMessage);
}

void NotioKonect::readyWrite()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if(reply == NULL)
    {
        MessageInfo("ERROR: NO REPLY");
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::readFileCompleted
///        This method is called when a downloading request is done and the file
///        is in our possession. It prepares the file to be saved.
///////////////////////////////////////////////////////////////////////////////////
void NotioKonect::readFileCompleted()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    // prepateResponse will rename the file if it converts to JSON.
    // So, we need to spot name changes to notify upstream that it did (e.g. FIT => JSON)
    // It also adds source files cloud ID to the ride file.
    QString wNewName = replyName(reply);
    QByteArray *wReturning = nullptr;

    if (reply->error() == QNetworkReply::NoError)
        wReturning = prepareResponse(buffers_.value(reply), wNewName);

    // return
    notifyReadComplete(wReturning, wNewName, tr("Completed."));
}

void NotioKonect::uploadFileDiagCompleted()
{
    replySuccess_ = false;

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray r = reply->readAll();
        qDebug() << "Request reoly with no error";
        qDebug() << QString("Request response: %1").arg(r.data());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // If there's an error just give up.
        if (parseError.error != QJsonParseError::NoError)
        {
            qDebug() << "json parse error: ....";
            qDebug() << QString("reply: %1").arg(r.data());
            MessageInfo("Json Parsing error");
        }
        else
        {
            if (document.object()["success"].isBool() == false || document.object()["success"].toBool() == false)
                MessageInfo("Request reply error (success=false)");
            else
                replySuccess_ = true;
        }
    }
    else
    {
        QByteArray r = reply->readAll();
        qDebug() << QString("Request error: %1[%2]").arg(QString::number(reply->error()), reply->errorString());
        qDebug() << "HTTP response code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
        qDebug() << "Request response:" << r.data();
        MessageInfo("Request reply error: " + reply->errorString());
    }
}

void NotioKonect::readyFileDiagUpload()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if (reply == NULL)
    {
        MessageInfo("Request returned NULL reply!!!");
        return;
    }
}

void NotioKonect::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if (reply == NULL)
    {
        MessageInfo("ERROR: NO REPLY");
        return;
    }

    buffers_.value(reply)->append(reply->readAll());
}

void NotioKonect::readyReadDir()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if (reply == NULL)
    {
        MessageInfo("ERROR: NO REPLY");
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::validateCredentials
///        This method validates Notio credentials.
///
/// \param[out] oErrors A string list reference containing errors.
/// \return An error.
///////////////////////////////////////////////////////////////////////////////////
NotioKonect::credentialError NotioKonect::validateCredentials(QStringList &oErrors)
{
    QString last_refresh_str =  getSetting(GC_NOTIOKONECT_LAST_ACCESS_TOKEN_REFRESH, "0").toString();
    QDateTime last_refresh = QDateTime::fromString(last_refresh_str, Qt::ISODate);
    if (!last_refresh.isValid())
    {
        oErrors << tr("Error reading Last access token from settings while trying to validate credentials.");
        return ERR_LAST_ACCESS_T_NOT_SET;
    }
    last_refresh = last_refresh.addSecs(24 * 60 * 60);

    QString refresh_token = QByteArray::fromBase64(getSetting(GC_NOTIOKONECT_REFRESH_TOKEN, "").toString().toUtf8());
    if (refresh_token == "")
    {
        oErrors << tr("ERROR: Getting invalid refresh token from settings while trying to validate credentials.");
        return ERR_REFRESH_T_NOT_SET;
    }

    // If we need to refresh the access token do so.
    QDateTime now = QDateTime::currentDateTime();
    if (now > last_refresh)
    {
        QByteArray wData;
#if QT_VERSION > 0x050000
        QUrlQuery wParams;
#else
        QUrl wParams;
#endif
        wParams.addQueryItem("grant_type", "refresh_token");
        wParams.addQueryItem("client_id", GC_NOTIOKONECT_CLIENT_ID);
        wParams.addQueryItem("refresh_token", refresh_token);
        wParams.addQueryItem("client_secret", GC_NOTIOKONECT_CLIENT_SECRET);

#if QT_VERSION > 0x050000
        wData.append(wParams.query(QUrl::FullyEncoded));
#else
        wData = wParams.encodedQuery();
#endif

        // access token request
        QUrl url = QUrl(QString("%1/api/v1/refresh").arg(NOTIOKONECT_HOST_URL));
        QNetworkRequest request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        QNetworkReply* reply = nam_->post(request, wData);

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        QByteArray r = reply->readAll();

        if (reply->error() != 0)
        {
            oErrors << tr("Request error while trying to validate credentials: ") + reply->errorString();
            return ERR_REPLY_ERROR;
        }

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        bool success = false;
        if(document.object()["success"].isBool()) {
            success = document.object()["success"].toBool();
        }

        // if path was returned all is good, lets set root
        if (parseError.error != QJsonParseError::NoError || success == false
                || document.object()["access_token"].isString() == false)
        {
            oErrors << tr("Error reading token while trying to validate credentials.");
            return ERR_JSON_ERROR;
        }

        QString access_token = document.object()["access_token"].toString();
        qDebug() << document.object()["refresh_token"].toString();

        // LOCALLY MAINTAINED -- WILL BE AN ISSUE IF ALLOW >1 ACCOUNT XXX
        setSetting(GC_NOTIOKONECT_ACCESS_TOKEN, access_token.toUtf8().toBase64());
        setSetting(GC_NOTIOKONECT_LAST_ACCESS_TOKEN_REFRESH, QDateTime::currentDateTime());
        CloudServiceFactory::instance().saveSettings(this, context);
    }

    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::getOAuthKey
///        This method generates a string of 16 cryptographically secure random
///        bytes.
///
/// \return A generated random alphanumeric key.
///////////////////////////////////////////////////////////////////////////////////
QString NotioKonect::getOAuthKey()
{
    QString wReturning;
    static const QString wAlphaNum = "0123456789"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz";
    // Generate 16 random bytes.
    for (int i = 0; i < 16; ++i)
    {
        wReturning.append(wAlphaNum[qrand() % (wAlphaNum.length() - 1)]);
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::computeOAuthChallengCode
///        This method generates a HMAC-SHA256 authentification code encoded in a
///        base64 URL friendly string.
///
/// \param[out] oOauthCodeVerifier  Generated verifier code.
/// \param[out] oOauthCodeChallenge Generated challenge code.
///////////////////////////////////////////////////////////////////////////////////
void NotioKonect::computeOAuthChallengCode(QString &oOauthCodeVerifier, QString &oOauthCodeChallenge)
{
    // Generate a string of 16 cryptographically secure random bytes
    QString wCryptoKey = getOAuthKey();

    // Encode the code into base64 URL friendly format.
    oOauthCodeVerifier = QString(wCryptoKey.toUtf8().toBase64()).split('=')[0];   // Remove Trailing '='
    oOauthCodeVerifier.replace('+', '-');
    oOauthCodeVerifier.replace('/', '_');

    // Generate challenge code using the client secret as a key.
    QMessageAuthenticationCode wSHA256(QCryptographicHash::Sha256);
    QByteArray wSecret = QByteArray::fromHex(QString(GC_NOTIOKONECT_CLIENT_SECRET).toLatin1());
    wSHA256.setKey(wSecret);
    wSHA256.addData(wCryptoKey.toUtf8());

    // Encode the challenge code into base64 URL friendly format.
    oOauthCodeChallenge = QString(wSHA256.result().toBase64()).split('=')[0];   // Remove trailing '='
    oOauthCodeChallenge.replace('+', '-');
    oOauthCodeChallenge.replace('/', '_');
}

QProgressDialog *NotioKonect::getProgressDialog() {
    if ((progressDialog_ == nullptr) && (context != nullptr)) {
       if (context->tab != nullptr)
       {
           progressDialog_ = new QProgressDialog(context->tab);
           progressDialog_->setRange(0, 0);
           progressDialog_->setValue(0);
           progressDialog_->setCancelButtonText(NULL);
           progressDialog_->setLabelText("---");
           progressDialog_->hide();
       }
    }
    return progressDialog_;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::MessageInfo
///        This method send a signal to display a information message box.
///
/// \param[in] what Message to display.
///////////////////////////////////////////////////////////////////////////////////
void NotioKonect::MessageInfo(QString iMessage)
{
    emit msgInfoBox(iMessage, "NotioKonect");
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::invalidateCredentials
///        This method invalidates the credentials when validating them fails.
///////////////////////////////////////////////////////////////////////////////////
void NotioKonect::invalidateCredentials()
{
    // Invalidate athlete's private Notio Konect credentials.
    appsettings->setCValue(context->athlete->cyclist, activeSettingName(), false);
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIOKONECT_ACCESS_TOKEN, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIOKONECT_REFRESH_TOKEN, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIOKONECT_LAST_ACCESS_TOKEN_REFRESH, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIOKONECT_ATHLETE_ID, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIOKONECT_ATHLETE_NAME, "");
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioKonect::actForUserID
///        This method creates an argument string to add to a URL request. It is
///        meant to add the athlete's ID when requesting data from the cloud.
///
///        Returns an empty string if we cannot find the user ID.
///
/// \param[in] iNewArg    Indicates to append the parameter to existing argument.
/// \return Argument string to add to a URL request for Notio Cloud.
///////////////////////////////////////////////////////////////////////////////////
QString NotioKonect::actForUserID(bool iAppendArg)
{
    // Get athlete's ID.
    QString wUserID = getSetting(GC_NOTIOKONECT_ATHLETE_ID, "").toString();
    QString wOutputStr;

    if (!wUserID.isEmpty())
    {
        // The string begins by '&' if it need to be append to existing parameters.
        // The string will begin by '?' if no arguments are already there.
        wOutputStr = (iAppendArg == APPEND_ARG) ? "&" : "?";
        wOutputStr.append("actfor=" + wUserID);
    }
    return wOutputStr;
}

static bool addNotioKonect()
{
    if (QString(GC_NOTIOKONECT_CLIENT_ID) == "__GC_NOTIOKONECT_CLIENT_ID__")
        return false;

    CloudServiceFactory::instance().addService(new NotioKonect(NULL));
    return true;
}

static bool add = addNotioKonect();
