#ifndef GC_NOTIOKONECT_H
#define GC_NOTIOKONECT_H

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QImage>

class QProgressDialog;
class QJsonObject;

#define NOTIOKONECT_HOST_URL "https://cloud.notiokonect.com"

class NotioKonect : public CloudService {

    Q_OBJECT

    public:
        explicit NotioKonect(Context *context);
        CloudService *clone(Context *context) { return new NotioKonect(context); }
        virtual ~NotioKonect();

        virtual QString id() const { return "NotioKonect"; }
        virtual QString uiName() const { return tr("Legacy Notio Konect"); }
        virtual QString description() const { return (tr("Sync your data via your legacy cloud storage. Coaching feature available.")); }
        QImage logo() const { return QImage(":images/services/notio.png"); }

        // upload only and authenticates with OAuth tokens
        int capabilities() const { return OAuth | Download | Query; }

        // open/connect and close/disconnect
        virtual bool open(QStringList &errors);
        virtual bool close();

        // athlete selection
        QList<CloudServiceAthlete> listAthletes();
        bool selectAthlete(CloudServiceAthlete);

        // home directory
        virtual QString home();

        // write a file
        virtual bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

        // read a file
        virtual bool readFile(QByteArray *data, QString remotename, QString);

        // Notio Konect needs the response to be adjusted before being imported
        QByteArray *prepareResponse(QByteArray *iData, QString &ioName);

        // create a folder
        virtual bool createFolder(QString path);

        // dirent style api
        virtual CloudServiceEntry *root() { return root_; }
        // Readdir reads the files from the remote side and updates root_dir_
        // with a local cache. readdir will read ALL files and refresh
        // everything.
        virtual QList<CloudServiceEntry*> readdir(QString path, QStringList &errors);

        virtual QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

        // Returns the fild id or "" if no file was found, uses the local
        // cache to determine file id.
        QString GetFileId(const QString& path);

        void uploadFileDialog();
        void uploadFile(QString fileName);

        typedef enum CredentialError {SUCCESS = 0, ERR_LAST_ACCESS_T_NOT_SET, ERR_REFRESH_T_NOT_SET,
                             ERR_REPLY_ERROR, ERR_JSON_ERROR} credentialError;

        credentialError validateCredentials(QStringList &oErrors);

        static QString getOAuthKey();
        static void computeOAuthChallengCode(QString&, QString&);

    public slots:

        // getting data
        void readyReadDir(); // get from to list
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

        // sending data
        void readyWrite();
        void writeFileCompleted();

        void readyFileDiagUpload();
        void uploadFileDiagCompleted();

        // dealing with SSL handshake problems
        void onSslErrors(QNetworkReply*reply, const QList<QSslError> & );

    signals:
        void msgInfoBox(QString, QString);

    private:
        struct FileInfo;

        QJsonObject ReadJsonElement(const QJsonArray &);

        // From QtForum: https://forum.qt.io/topic/25096/modify-nested-qjsonvalue/4
        //usage:
        //    modifyJsonValue(doc, "firstName", QJsonValue("Natalia"));
        //    modifyJsonValue(doc, "age", 22);
        //    modifyJsonValue(doc, "address.state", "None");
        //    modifyJsonValue(doc, "phoneNumber[0].number", "333 543-3210");
        //    modifyJsonValue(doc, "family[0][2]", "Bill");
        //    modifyJsonValue(doc, "family[1][1]", "Winston");
        //    modifyJsonValue(doc, "family[2].father.age", 56);
        void modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue);
        void modifyJsonValue(QJsonDocument& doc, const QString& path, const QJsonValue& newValue);

        void MessageInfo(QString iMessage);

        FileInfo* BuildDirectoriesForAthleteDirectory(const QString& path);

        void invalidateCredentials();

        static QNetworkRequest MakeRequestWithURL(const QString& url, const QString& token, const QString& args);
        static const bool APPEND_ARG = true;
        QString actForUserID(bool iAppendArg = false);

        QNetworkAccessManager *nam_;

        bool replySuccess_;
        CloudServiceEntry *root_;
        QScopedPointer<FileInfo> root_dir_;

        QMap<QNetworkReply*, QByteArray*> buffers_;
        QMap<QString, QJsonObject> m_replyActivity;

        // Cannot create in constructor as Application is not yet created.
        QProgressDialog *progressDialog_;
        QProgressDialog *getProgressDialog();

    public:
        enum eLoadType {A18_UNKNOWN = 0, A18_BCV4, A18_PLANNED, A18_FIT, A18_MERGED,
                        A18_EXECUTED_BCV4, A18_EXECUTED_FIT, A18_EXECUTED_MERGED, A18_PLANNED_TEMP};
        typedef enum eLoadType LoadType;
        LoadType currentLoad() const { return loadType; }
    protected:
        LoadType loadType;
};

#endif

