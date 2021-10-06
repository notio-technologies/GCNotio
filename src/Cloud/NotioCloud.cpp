/*
 * Copyright (c) 2019 Michaël Beaulieu (michael.beaulieu@notiotechnologies.com)
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

#include "NotioCloud.h"
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

QString NotioCloud::kNotioApiBaseAddr = "https://cloud.notio.ai";
QString NotioCloud::kNotioApiActivities = "/activity/v1/activities";
QString NotioCloud::kNotioApiActivityData = "/activity/v1/activities/data/";

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::NotioCloud
///        Constructor.
///
/// \param[in] iContext  Context.
///////////////////////////////////////////////////////////////////////////////
NotioCloud::NotioCloud(Context *iContext) : CloudService(iContext)
{
    if (iContext)
    {
        m_nam = new QNetworkAccessManager(this);
        connect(m_nam, SIGNAL(sslErrors(QNetworkReply *, const QList<QSslError> &)), this, SLOT(onSslErrors(QNetworkReply *, const QList<QSslError> &)));
        connect(this, SIGNAL(msgInfoBox(QString, QString)), reinterpret_cast<QObject *>(const_cast<MainWindow *>(iContext->mainWindow)),
                SLOT(messageInfoBox(QString, QString)), Qt::QueuedConnection);
    }

    // Setup upload and download compression.
    uploadCompression = none;
    downloadCompression = none;

    // Use metric distance and duration metadata (Sync tab).
    useMetric = true;

    // Connection config.
    settings.insert(OAuthToken, GC_NOTIO_ACCESS_TOKEN);
    settings.insert(Local1, GC_NOTIO_REFRESH_TOKEN);
    settings.insert(Local2, GC_NOTIO_LAST_ACCESS_TOKEN_REFRESH);
    settings.insert(Local3, GC_NOTIO_SIGN_IN);                   // Sign-in time with credentials.
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::~NotioCloud
///        Destructor.
///////////////////////////////////////////////////////////////////////////////
NotioCloud::~NotioCloud()
{
    qDebug() << Q_FUNC_INFO;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::open
///        This method is call upon opening and connecting to cloud service. It
///        verifies the credentials. It also requests the athletes' list and
///        selects the current athlete from the settings else select the Notio
///        account owner.
///
/// \param[out] oErrors Error message.
/// \return Status
///////////////////////////////////////////////////////////////////////////////////
bool NotioCloud::open(QStringList &oErrors)
{
    bool wReturning = true;

    // Check credentials.
    if (validateCredentials(oErrors) != SUCCESS)
    {
        oErrors << tr("You must authorise with Notio first. Add a Notio cloud account.");
        invalidateCredentials();
        return false;
    }

    // Check for access token.
    QString wToken = decryptToken(getSetting(GC_NOTIO_ACCESS_TOKEN, "").toString());
    if (wToken.isEmpty())
    {
        oErrors << tr("No token access after connection check! Verify your Notio credentials.");
        return false;
    }

    return wReturning;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::close
///        This method closes the service.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
bool NotioCloud::close()
{
    // Nothing to do for now.
    qDebug() << Q_FUNC_INFO;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::home
///        This method get the notio home directory.
///
/// \return The home directory string.
///////////////////////////////////////////////////////////////////////////////
QString NotioCloud::home()
{
    qDebug() << Q_FUNC_INFO;
    return getSetting(GC_NOTIO_FOLDER, "").toString();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::readFile
///        This method constructs the network request to download an activity from
///        the cloud server.
///
/// \param[out] oData       Raw data extracted
/// \param[in]  iRemoteName Activity filename.
/// \param[in]  iRemoteId   Activity cloud storage ID. (Not used)
/// \return A status.
///////////////////////////////////////////////////////////////////////////////////
bool NotioCloud::readFile(QByteArray *oData, QString iRemoteName, QString iRemoteId)
{
    // Validate credentials.
    QStringList wErrors;
    if (validateCredentials(wErrors) != SUCCESS)
    {
        wErrors << tr("You must authorise with Notio first. Add a Notio cloud account.");
        MessageInfo(wErrors.join("\n"));
        return false;
    }

    // Get the access token.
    QString wToken = decryptToken(getSetting(GC_NOTIO_ACCESS_TOKEN, "").toString());
    if (wToken == "")
    {
        MessageInfo("No token access after connection check! Verify your Notio credentials.");
        return false;
    }

    // Prepare request.
    QNetworkRequest wRequest = MakeRequestWithURL(kNotioApiActivityData, wToken, iRemoteId);

    // Get the file.
    QNetworkReply *wReply = m_nam->get(wRequest);

    // Remember the file.
    mapReply(wReply, iRemoteName);
    m_buffers.insert(wReply, oData);

    connect(wReply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(wReply, SIGNAL(readyRead()), this, SLOT(readyRead()));

    if (wReply->error() != QNetworkReply::NoError)
    {
        qDebug() << "There is an error getting the file from Notio Cloud server." << wReply->error() << wReply->errorString();
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::prepareReceivedFile
///        This methods prepares the data received for a dowloaded activity. It
///        converts any file received into the GC JSON format. It also adds some
///        information about the metadata received.
///
/// \param[in, out] ioData  Raw data received.
/// \param[in, out] ioName  Name of the activity.
///////////////////////////////////////////////////////////////////////////////////
void NotioCloud::prepareReceivedFile(QByteArray *ioData, QString &ioName)
{
    qDebug() << Q_FUNC_INFO;

    // Uncompress, parse and convert the activity.
    QStringList wErrors;
    RideFile *wRide = uncompressRide(ioData, ioName, wErrors);
    if (wRide)
    {
        // Get the metadata of the activity.
        QJsonObject wActivity = m_replyActivity.value(ioName, QJsonObject());
        QJsonObject wMetadata = wActivity["metadata"].toObject();

        // Set Source cloud IDs.
        if (!wActivity["id"].isNull())
            wRide->setTag("Cloud ID", wActivity["id"].toString());

        // Set sport.
        if (wRide->getTag("Sport", "").isEmpty())
            wRide->setTag("Sport", "Bike");

        // Set title.
        if (wRide->getTag("title", "").isEmpty())
            wRide->setTag("title", wMetadata["title"].toString());

        // Set calibration ride flag.
        if (!wMetadata["isCalibration"].isNull())
            wRide->setTag("notio.calibrationRide", wMetadata["isCalibration"].toBool() ? "true" : "false");

        // Set creation timestamp.
        if (!wActivity["creationTimestamp"].isNull())
            wRide->setTag("Cloud Creation Timestamp", wActivity["creationTimestamp"].toString());

        // Set last update timestamp.
        if (!wActivity["lastUpdateTimestamp"].isNull())
            wRide->setTag("Cloud Last Update Timestamp", wActivity["lastUpdateTimestamp"].toString());

        // Convert JSON to byte array.
        JsonFileReader wReader;
        ioData->clear();
        ioData->append(wReader.toByteArray(context, wRide, true, true, true, true));

        // Rename the file to have the JSON extension.
        if (QFileInfo(ioName).suffix() != "json") ioName = QFileInfo(ioName).baseName() + ".json";
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::readdir
///        This method retrieves the list of files stored on the cloud.
///
/// \param[in]  iPath   Where the files are located on the cloud. (Not used)
/// \param[out] oErrors Errors' list of the operation.
/// \param[in]  iFrom   Starting date.  (Not used)
/// \param[in]  iTo     Ending date.    (Not used)
///
/// \return Entries' list.
///////////////////////////////////////////////////////////////////////////////
QList<CloudServiceEntry*> NotioCloud::readdir(QString iPath, QStringList &oErrors, QDateTime iFrom, QDateTime iTo)
{
    qDebug() << Q_FUNC_INFO << iFrom.toString("yy-MM-dd") << iTo.toString("yy-MM-dd");

    Q_UNUSED(iPath)

    QMap<QString, CloudServiceEntry*> wReturning;

    // Get access token.
    QString wToken = decryptToken(getSetting(GC_NOTIO_ACCESS_TOKEN, "").toString());
    if (wToken == "")
    {
        oErrors << tr("No token access after connection check!!!");
        return wReturning.values();
    }

    // Request activities' list.
    QNetworkRequest wRequest = MakeRequestWithURL(kNotioApiActivities, wToken, "");
    QNetworkReply *wReply = m_nam->get(wRequest);

    // Blocking request while reading activities' list.
    QEventLoop wLoop;
    connect(wReply, SIGNAL(finished()), &wLoop, SLOT(quit()));
    wLoop.exec();

    QByteArray wRead = wReply->readAll();

    // Check for errors.
    if (wReply->error() != QNetworkReply::NoError)
    {
        qDebug() << QString("Request error: %1[%2]").arg(QString::number(wReply->error()), wReply->errorString());
        qDebug() << "HTTP response code:" << wReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
        qDebug() << "Request response:" << wRead.data();

        // Parse and display reply error.
        QByteArray wNotioCloudError = wRead.data();
        QJsonParseError wParseError;
        QJsonDocument wDocument = QJsonDocument::fromJson(wNotioCloudError, &wParseError);

        if ((wParseError.error == QJsonParseError::NoError) && (wDocument.object()["error"].toString().isNull() == false))
        {
            QString wErrorString = wDocument.object()["error"].toString();
            QString wErrorMessage = wDocument.object()["message"].toString();
            int wErrorCode = wDocument.object()["code"].toInt();
            QJsonArray wErrorDetails = wDocument.object()["details"].toArray();
            QString wDetails;
            for (auto wArrayItr : wErrorDetails)
                wDetails += "\n" + wArrayItr.toString();

            QMessageBox wMsgBox;
            wMsgBox.setIcon(QMessageBox::Critical);
            wMsgBox.setModal(true);
            wMsgBox.setWindowTitle(tr("Request (from-to)"));
            wMsgBox.setText(wErrorString);
            wMsgBox.setDetailedText(QString(tr("Code %1: %2%3")).arg(QString::number(wErrorCode), wErrorMessage, wDetails));
            wMsgBox.exec();
        }
        else
            MessageInfo("Request(from-to) reply error: " + wReply->errorString());

        return wReturning.values();
    }

    QJsonParseError wParseError;
    QJsonDocument document = QJsonDocument::fromJson(wRead, &wParseError);

    // If there's an error just give up.
    if (wParseError.error != QJsonParseError::NoError)
    {
        qDebug() << "Json parse error: ....";
        qDebug() << "Reply:" << wRead.data();
        oErrors << tr("Json Parsing error");

        return wReturning.values();
    }

    // Extract activities' metadata list.
    QJsonArray wResultArray = document.object()["activities"].toArray();

    if(wResultArray.isEmpty())
    {
        qDebug() << "No files to download";
        qDebug() << "Reply:" << wRead.data();
        return wReturning.values();
    }

    // Go through all activities.
    for (auto wActivityItr : wResultArray)
    {
        // Activity data.
        QJsonObject wObj = wActivityItr.toObject();

        // Activity metadata.
        QJsonObject wMetadata = wObj["metadata"].toObject();

        // Get activity start time.
        QDateTime wStartTimestamp = QDateTime::fromTime_t(wMetadata["startTimestamp"].toString().toUInt(), Qt::UTC);

        if (wMetadata["editedStartTimestamp"].toString().toUInt() > 0)
            wStartTimestamp = QDateTime::fromTime_t(wMetadata["editedStartTimestamp"].toString().toUInt(), Qt::UTC);

        // Get file type.
        QString wSuffix = "json";
        QStringList wDeviceList = { wMetadata["notioId"].toString() };

        // To Implement - Manage Notio, FIT and GCJSON file types. Maybe add binary and csv file types.
        // For the Devices list, we'll be missing GarminId.

        // Activity datatype not recognized or no start date.
        iTo.setTime(QTime(23, 59, 59, 0));
        if (wSuffix.isEmpty() || (wStartTimestamp.toLocalTime() < iFrom) || (wStartTimestamp.toLocalTime() > iTo))
            continue;

        CloudServiceEntry *wNewActivity = newCloudServiceEntry();

        // Populate Activity cloud info.
        wNewActivity->id = wObj["id"].toString();
        wNewActivity->modified = QDateTime::fromTime_t(wObj["lastUpdateTimestamp"].toString().toUInt(), Qt::LocalTime);
        wNewActivity->type = wSuffix;

        // Populate Activity metadata.
        wNewActivity->distance = wMetadata["distanceInKm"].toDouble();
        wNewActivity->label = wMetadata["title"].toString();
        wNewActivity->devices = wDeviceList.join("; ");

        // Calculate duration.
        double wStopTime = wMetadata["endTimestamp"].toString().toDouble();
        double wStartTime = wMetadata["startTimestamp"].toString().toDouble();

        if (wStopTime > wStartTime)
        {
            wNewActivity->duration = static_cast<long>(wStopTime - wStartTime);
        }

        // Populate other data.
        wNewActivity->sport = tr("Bike");       // Need to be confirmed when Garmin FIT files will be added.
        wNewActivity->isDir = false;
        wNewActivity->size = 0;

        wNewActivity->name = wStartTimestamp.toLocalTime().toString("yyyy_MM_dd_hh_mm_ss") + "." + wSuffix;

        // The GC sync dialog uses the activity name to know which file to download.
        m_replyActivity.insert(wNewActivity->name, wObj);

        wReturning.insert(wNewActivity->name, wNewActivity);
    }

    return wReturning.values();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::getOAuthKey
///        This method generates a string of 16 cryptographically secure random
///        bytes.
///
/// \return A generated random alphanumeric key.
///////////////////////////////////////////////////////////////////////////////////
QString NotioCloud::getOAuthKey()
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

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::getCryptoKey
///        This method creates a key of a specified length used to encode or
///        decode a string.
///
/// \param[in] iSize    Length of the key.
/// \return A byte array key.
///////////////////////////////////////////////////////////////////////////////
QByteArray NotioCloud::getCryptoKey(const int iSize)
{
    // Get the date the user sign in to authorize GC connection.
    QDateTime wSignIn = QDateTime::fromString(getSetting(GC_NOTIO_SIGN_IN, "").toString(), Qt::ISODate);

    // Initialize the seed of qsrand.
    qsrand(static_cast<uint>(wSignIn.toSecsSinceEpoch()));

    QString wReturning;
    int wSecretMiddle = QString(GC_NOTIO_CLIENT_SECRET).size() / 2;
    static const QString wAlphaNum = QString(GC_NOTIO_CLIENT_SECRET).left(wSecretMiddle) +
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "0123456789"
                                     "abcdefghijklmnopqrstuvwxyz" +
                                     QString(GC_NOTIO_CLIENT_SECRET).mid(wSecretMiddle);

    // Generate x random bytes.
    for (int i = 0; i < iSize; ++i)
    {
        wReturning.append(wAlphaNum[qrand() % (wAlphaNum.length() - 1)]);
    }

    return wReturning.toUtf8();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::computeOAuthChallengCode
///        This method generates a HMAC-SHA256 authentification code encoded in a
///        base64 URL friendly string.
///
/// \param[out] oOauthCodeVerifier  Generated verifier code.
/// \param[out] oOauthCodeChallenge Generated challenge code.
///////////////////////////////////////////////////////////////////////////////////
void NotioCloud::computeOAuthChallengCode(QString &oOauthCodeVerifier, QString &oOauthCodeChallenge)
{
    // Generate a string of 16 cryptographically secure random bytes
    QString wCryptoKey = getOAuthKey();

    // Encode the code into base64 URL friendly format.
    oOauthCodeVerifier = QString(wCryptoKey.toUtf8().toBase64()).split('=')[0];   // Remove Trailing '='
    oOauthCodeVerifier.replace('+', '-');
    oOauthCodeVerifier.replace('/', '_');

    // Generate challenge code using the client secret as a key.
    QMessageAuthenticationCode wSHA256(QCryptographicHash::Sha256);
    QByteArray wSecret = QByteArray::fromHex(QString(GC_NOTIO_CLIENT_SECRET).toLatin1());
    wSHA256.setKey(wSecret);
    wSHA256.addData(wCryptoKey.toUtf8());

    // Encode the challenge code into base64 URL friendly format.
    oOauthCodeChallenge = QString(wSHA256.result().toBase64()).split('=')[0];   // Remove trailing '='
    oOauthCodeChallenge.replace('+', '-');
    oOauthCodeChallenge.replace('/', '_');
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::validateCredentials
///        This method validates Notio credentials.
///
/// \param[out] oErrors A string list reference containing errors.
/// \return An error.
///////////////////////////////////////////////////////////////////////////////////
NotioCloud::eCredentialError NotioCloud::validateCredentials(QStringList &oErrors)
{
    qDebug() << Q_FUNC_INFO;

    // Validate last refresh timestamp.
    QString wLastRefreshSetting =  getSetting(GC_NOTIO_LAST_ACCESS_TOKEN_REFRESH, "0").toString();
    QDateTime wLastRefresh = QDateTime::fromString(wLastRefreshSetting, Qt::ISODate);
    if(!wLastRefresh.isValid())
    {
        qDebug() << "ERROR: Getting invalid date from GC_NOTIO_LAST_ACCESS_TOKEN_REFRESH setting";
        oErrors << tr("Error reading Last access token from settings while trying to validate credentials.");
        return ERR_LAST_ACCESS_T_NOT_SET;
    }

    // The access token expire after 1 hour. So, refresh if last refresh was more than 55 minutes.
    wLastRefresh = wLastRefresh.addSecs(55 * 60);

    // Get refresh token validate it.
    QString wRefreshToken = decryptToken(getSetting(GC_NOTIO_REFRESH_TOKEN, "").toString());
    if (wRefreshToken == "")
    {
        qDebug() << "ERROR: GC_NOTIO_REFRESH_TOKEN is not set";
        oErrors << tr("ERROR: Getting invalid refresh token from settings while trying to validate credentials.");
        return ERR_REFRESH_T_NOT_SET;
    }

    // If we need to refresh the access token do so.
    QDateTime wNow = QDateTime::currentDateTime();
    if (wNow > wLastRefresh)
    {
        qDebug() << "will refresh credentials";

        QByteArray wData;

        QJsonObject wJsonParams;
        wJsonParams.insert("GrantType", "refresh_token");
        wJsonParams.insert("ClientId", GC_NOTIO_CLIENT_ID);
        wJsonParams.insert("RefreshToken", wRefreshToken);
        wJsonParams.insert("ClientSecret", GC_NOTIO_CLIENT_SECRET);

        wData.append(QJsonDocument(wJsonParams).toJson());

        // Access token request.
        QUrl wUrl = QUrl(QString("%1/oauth/v1/refreshtoken").arg(kNotioApiBaseAddr));
        QNetworkRequest wRequest = QNetworkRequest(wUrl);
        wRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply* wReply = m_nam->post(wRequest, wData);

        // Blocking request.
        QEventLoop wLoop;
        connect(wReply, SIGNAL(finished()), &wLoop, SLOT(quit()));
        wLoop.exec();

        QByteArray wRead = wReply->readAll();

        // Check for errors.
        if (wReply->error() != 0)
        {
            qDebug() << QString("Request error: %1[%2]").arg(QString::number(wReply->error()), wReply->errorString());
            qDebug() << "HTTP response code:" << wReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
            qDebug() << QString("Request response: %1\n").arg(wRead.data());
            oErrors << tr("Request error while trying to validate credentials: ") + wReply->errorString();
            return ERR_REPLY_ERROR;
        }

        // Validate successful request.
        QJsonParseError wParseError;
        QJsonDocument wDocument = QJsonDocument::fromJson(wRead, &wParseError);
        if (wParseError.error != QJsonParseError::NoError || wDocument.object()["success"].toBool() == false
                || wDocument.object()["access_token"].isString() == false)
        {
            qDebug() << "Parse error!";
            qDebug() << "Got response:" << wRead.data();
            oErrors << tr("Error reading token while trying to validate credentials.");
            return ERR_JSON_ERROR;
        }

        // Save new access and refresh tokens.
        QString AccessToken = wDocument.object()["access_token"].toString();
        wRefreshToken = wDocument.object()["refresh_token"].toString();

        setSetting(GC_NOTIO_ACCESS_TOKEN, encryptToken(AccessToken));
        setSetting(GC_NOTIO_LAST_ACCESS_TOKEN_REFRESH, QDateTime::currentDateTime());

        // Save refresh token only if server gave one.
        if (wRefreshToken.isEmpty() == false)
            setSetting(GC_NOTIO_REFRESH_TOKEN, encryptToken(wRefreshToken));

        CloudServiceFactory::instance().saveSettings(this, context);
        qDebug() << "Reconnected successfully.";
    }
    else
    {
        qDebug() << "Credentials are still good, not refreshing.";
    }

    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::encodeToken
///        This method encrypts a token with a cryptographic key.
///
/// \param[in] iToken   Token to encode.
///
/// \return The encoded token string.
///////////////////////////////////////////////////////////////////////////////
QString NotioCloud::encryptToken(QString iToken)
{
    QByteArray wToken = iToken.toUtf8();

    // Generate cryptographically secure random bytes of the token size.
    QByteArray wCryptoKey = getCryptoKey(wToken.size());

    // Encrypt each token byte.
    for (int i = 0; i < wToken.size(); i++)
    {
        wToken[i] = wToken[i] ^ wCryptoKey[i];
    }

    // Return encrypted string encoded to Base64.
    return wToken.toBase64();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::decodeToken
///        This method decrypts an encrypted token to a simple string.
///
/// \param[in] iEncrypToken Encrypted token.
///
/// \return The token as a string.
///////////////////////////////////////////////////////////////////////////////
QString NotioCloud::decryptToken(QString iEncrypToken)
{
    // Decode from Base64 the encrypted token.
    QByteArray wEncryptedToken = QByteArray::fromBase64(iEncrypToken.toUtf8());

    // Generate cryptographically secure random bytes of the token size.
    QByteArray wCryptoKey = getCryptoKey(wEncryptedToken.size());

    // Decrypt each token byte.
    for (int i = 0; i < wEncryptedToken.size(); i++)
    {
        wEncryptedToken[i] = wEncryptedToken[i] ^ wCryptoKey[i];
    }

    // Return the decrypted token.
    return wEncryptedToken;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::readyRead
///        This method is called when a the network request to download a ride
///        is done and we are ready to read the data.
///////////////////////////////////////////////////////////////////////////////
void NotioCloud::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if(reply == nullptr)
    {
        MessageInfo("ERROR: NO REPLY");
        return;
    }

    m_buffers.value(reply)->append(reply->readAll());
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::readFileCompleted
///        This method is called when a downloading request is done and the file
///        is in our possession. It prepares the file to be saved.
///////////////////////////////////////////////////////////////////////////////////
void NotioCloud::readFileCompleted()
{
    qDebug() << Q_FUNC_INFO;

    QString wMessage = tr("Network reply error");
    QString wNewName;
    QByteArray *wReturning = nullptr;

    // Get the network reply.
    QNetworkReply *wReply = static_cast<QNetworkReply*>(QObject::sender());
    if (wReply && (wReply->error() == QNetworkReply::NoError))
    {
        // Get activity name and data.
        wNewName = replyName(wReply);
        wReturning = m_buffers.value(wReply);

        if (wReturning)
        {
            prepareReceivedFile(wReturning, wNewName);
            wMessage = tr("Completed");
        }
        else
        {
            wMessage = tr("Error data corrupted.");
        }
    }
    else
    {
        wMessage.append(QString(": %1").arg(wReply->error()));
        qDebug() << wMessage << "-" << wReply->errorString();
    }

    notifyReadComplete(wReturning, wNewName, wMessage);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::onSslErrors
///        This method is called upon SSL errors.
///
/// \param[in] reply    Network reply.
///////////////////////////////////////////////////////////////////////////////
void NotioCloud::onSslErrors(QNetworkReply*reply, const QList<QSslError> & )
{
    qDebug() << Q_FUNC_INFO;
    reply->ignoreSslErrors();
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::MessageInfo
///        This method send a signal to display a information message box.
///
/// \param[in] what Message to display.
///////////////////////////////////////////////////////////////////////////////////
void NotioCloud::MessageInfo(QString iMessage)
{
    emit msgInfoBox(iMessage, "Notio Cloud");
}

///////////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::invalidateCredentials
///        This method invalidates the credentials when validating them fails.
///////////////////////////////////////////////////////////////////////////////////
void NotioCloud::invalidateCredentials()
{
    // Invalidate athlete's private Notio credentials.
    appsettings->setCValue(context->athlete->cyclist, activeSettingName(), false);
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIO_ACCESS_TOKEN, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIO_REFRESH_TOKEN, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIO_LAST_ACCESS_TOKEN_REFRESH, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIO_SIGN_IN, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIO_ATHLETE_ID, "");
    appsettings->setCValue(context->athlete->cyclist, GC_NOTIO_ATHLETE_NAME, "");
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::MakeRequestWithURL
///        This method makes a request with specified URL and parameters.
///
/// \param[in] iUrl      Request URL.
/// \param[in] iToken    Access token required by the cloud service.
/// \param[in] iArgs     Arguments to pass in the network request.
///
/// \return The Network request.
///////////////////////////////////////////////////////////////////////////////
QNetworkRequest NotioCloud::MakeRequestWithURL(const QString &iUrl, const QString &iToken, const QString &iArgs)
{
    qDebug() << Q_FUNC_INFO;
    QString request_url;
    request_url =  kNotioApiBaseAddr + iUrl + iArgs;

    QNetworkRequest request(request_url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(iToken)).toLatin1());

    if (iUrl == kNotioApiActivityData)
    {
        request.setRawHeader("Accept", "application/notio-gc+json");
        //request.setRawHeader("Accept", "application/notio-gc+json, application/notio-csv+text");  // Multiple accepted format.
    }

    return request;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief NotioCloud::getProgressDialog
///        This method creates a progress dialog.
///
/// \return A pointer to the progress dialog.
///////////////////////////////////////////////////////////////////////////////
QProgressDialog *NotioCloud::getProgressDialog()
{
    if ((progressDialog_ == nullptr) && (context != nullptr)) {
       if (context->tab != nullptr)
       {
           progressDialog_ = new QProgressDialog(context->tab);
           progressDialog_->setRange(0, 0);
           progressDialog_->setValue(0);
           progressDialog_->setCancelButtonText(nullptr);
           progressDialog_->setLabelText("---");
           progressDialog_->hide();
       }
    }
    return progressDialog_;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief addNotioCloud
///        This method creates an instance of the cloud service.
///
/// \return The status of the operation.
///////////////////////////////////////////////////////////////////////////////
static bool addNotioCloud()
{
    if (QString(GC_NOTIO_CLIENT_ID) == "__GC_NOTIO_CLIENT_ID__")
        return false;

    CloudServiceFactory::instance().addService(new NotioCloud(nullptr));
    return true;
}

/// Create a Notio cloud instance.
static bool add = addNotioCloud();
