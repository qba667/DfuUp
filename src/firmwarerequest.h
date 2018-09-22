#ifndef FIRMWAREREQUEST_H
#define FIRMWAREREQUEST_H
#include <QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QSslError>

class FirmwareRequest : public QObject
{
        Q_OBJECT
public:
    enum Result{
        Error,
        Success
    };

    FirmwareRequest();
    ~FirmwareRequest();
    void setUID(const QString &uid);
    void getFirmwareList();
signals:
    void progress(const QString& message, int progress);
    void done(const QString& message, FirmwareRequest::Result status, QJsonDocument* result);
private slots:
    void managerFinished(QNetworkReply *reply);
    void onIgnoreSSLErrors(QNetworkReply* reply, const QList<QSslError> &errors);
private:
    QJsonDocument doc;
    QString* uid;
    QNetworkAccessManager *manager;
    const QString Progress_CHECKIG = QString("Checking for updates.");
    const QString Progress_SENDING = QString("Sending request.");
    const QString Progress_RECIEVING = QString("Recieving response.");
    const QString Progress_CHECKINGERROR = QString("Checking errors.");
    const QString Progress_PARSING_RESP = QString("Parsing response.");

    const QString Response_INVALID_SERV = QString("Invalid server response.");

    const QString repositoryURL = QString("https://update.nv14.local/firmware.json");
    const QVariant clinetAgent = QVariant("NV14-update-tool");
    const QString queryUID = QString("deviceUID");
};

#endif // FIRMWAREREQUEST_H
