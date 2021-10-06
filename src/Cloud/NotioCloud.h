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

#ifndef GC_NOTIOCLOUD_H
#define GC_NOTIOCLOUD_H

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QImage>

class QProgressDialog;
class QJsonObject;

///////////////////////////////////////////////////////////////////////////////
/// \brief The NotioCloud class
///        This class defines the Notio cloud service.
///////////////////////////////////////////////////////////////////////////////
class NotioCloud : public CloudService {

    Q_OBJECT

    typedef enum CredentialError { SUCCESS = 0, ERR_LAST_ACCESS_T_NOT_SET, ERR_REFRESH_T_NOT_SET,
                                   ERR_REPLY_ERROR, ERR_JSON_ERROR } eCredentialError;

public:
    explicit NotioCloud(Context *iContext);
    CloudService *clone(Context *iContext) { return new NotioCloud(iContext); }
    ~NotioCloud();

    // Getters.
    QString id() const { return "NotioCloud"; }
    QString uiName() const { return tr("Notio"); }
    QString description() const { return (tr("Sync your data via your cloud storage. Coaching feature not yet available.")); }
    QImage logo() const { return QImage(":images/services/notio.png"); }

    // Download only and authenticates with OAuth tokens.
    int capabilities() const { return OAuth | Download | Query; }

    // Open/connect and close/disconnect.
    bool open(QStringList &oErrors);
    bool close();

    // Home directory
    QString home();

    // Read a file.
    bool readFile(QByteArray *oData, QString iRemoteName, QString);

    // Updates GCJson file metadata.
    void prepareReceivedFile(QByteArray *iData, QString &ioName);

    // Readdir reads the files from the remote side and updates root_dir_
    // with a local cache.
    QList<CloudServiceEntry*> readdir(QString iPath, QStringList &oErrors, QDateTime iFrom, QDateTime iTo);

    // OAuth
    static QString getOAuthKey();
    QByteArray getCryptoKey(const int iSize);
    static void computeOAuthChallengCode(QString&, QString&);
    eCredentialError validateCredentials(QStringList &oErrors);

    // Token encryption.
    QString encryptToken(QString iToken);
    QString decryptToken(QString iEncrypToken);

    static QString kNotioApiBaseAddr;

public slots:
    // Read data.
    void readyRead();
    void readFileCompleted();

    // SSL handshake errors
    void onSslErrors(QNetworkReply*reply, const QList<QSslError> & );

signals:
    void msgInfoBox(QString, QString);

private:
    void MessageInfo(QString iMessage);

    void invalidateCredentials();
    static QNetworkRequest MakeRequestWithURL(const QString &iUrl, const QString &iToken, const QString &iArgs);

    // Cannot create in constructor as Application is not yet created.
    QProgressDialog *progressDialog_ = nullptr;
    QProgressDialog *getProgressDialog();

    QNetworkAccessManager *m_nam = nullptr;
    QMap<QNetworkReply*, QByteArray*> m_buffers;
    QMap<QString, QJsonObject> m_replyActivity;

    // API Url strings.
    static QString kNotioApiActivities;
    static QString kNotioApiActivityData;
};

#endif // GC_NOTIOCLOUD_H
