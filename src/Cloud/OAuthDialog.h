/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef OAUTHDIALOG_H
#define OAUTHDIALOG_H
#include "GoldenCheetah.h"
#include "Pages.h"
#include "CloudService.h"
#include <QObject>
#include <QtGui>
#include <QWidget>
#include <QStackedLayout>
#include <QUrl>
#include <QSslSocket>
#ifndef NOWEBKIT
#include <QtWebKit>
#include <QWebView>
#include <QWebFrame>
#endif

// QUrl split into QUrlQuerty in QT5
#if QT_VERSION > 0x050000
#include <QUrlQuery>
#endif
// QWebEngine if on Mac, -or- we don't have webkit
#if defined(NOWEBKIT) || ((QT_VERSION > 0x050000) && defined(Q_OS_MAC))
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QWebEngineProfile>
#if (QT_VERSION >= 0x050600)
#include <QWebEngineCookieStore>
#endif
#endif


class OAuthDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    typedef enum {
        NONE=0,
        STRAVA,
        DROPBOX,
        CYCLING_ANALYTICS,
        GOOGLE_CALENDAR,
        GOOGLE_DRIVE,
        SPORTTRACKS,
        TODAYSPLAN,
        WITHINGS,
        POLAR,
        NOTIOKONECT,
        KENTUNI,
        XERT,
        NOTIOCLOUD
    } OAuthSite;

    // will work with old config via site and new via cloudservice (which is null for calendar and withings for now)
    OAuthDialog(Context *context, OAuthSite site, CloudService *service, QString baseURL="", QString clientsecret="");

    bool sslLibMissing() { return noSSLlib; }

private slots:
    // Strava/Cyclinganalytics/Google
    void urlChanged(const QUrl& url);
    void loadFinished(bool ok);
    void networkRequestFinished(QNetworkReply *reply);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);


private:
    Context *context;
    bool noSSLlib;
    bool ignore;
    OAuthSite site;
    CloudService *service;
    QString baseURL; // can be passed, but typically is blank (used by Todays Plan)
    QString clientsecret; // can be passed, but typicall is blank (used by Todays Plan)

    QVBoxLayout *layout;

    // QUrl split into QUrlQuerty in QT5
#if defined(NOWEBKIT) || ((QT_VERSION > 0x050000) && defined(Q_OS_MAC))
    QWebEngineView *view;
#else
    QWebView *view;
#endif

    QNetworkAccessManager* manager;

    QUrl url;

    QString m_oAuthNkState;
    QString m_oAuthNkCodeVerifier;
    QString m_oAuthNkCodeChallenge;
};

#endif // OAUTHDIALOG_H
