#include "firmwarerequest.h"

FirmwareRequest::FirmwareRequest()
{
    this->manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(managerFinished(QNetworkReply*)));
    connect(manager,SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),this,SLOT(onIgnoreSSLErrors(QNetworkReply*, const QList<QSslError>&)));
}
FirmwareRequest::~FirmwareRequest()
{
    delete uid;
    delete manager;
}
void FirmwareRequest::setUID(const QString &uid){
    this->uid = new QString(uid);
}
void FirmwareRequest::onIgnoreSSLErrors(QNetworkReply* reply, const QList<QSslError> &errors){
    if(manager == nullptr) return;
    foreach (const QSslError &error, errors)
        qDebug() << "skip SSL error: " << qPrintable(error.errorString());
    reply->ignoreSslErrors(errors);
}


void FirmwareRequest::managerFinished(QNetworkReply *reply){
    if(manager == nullptr) return;
    emit progress(FirmwareRequest::Progress_RECIEVING, 60);
    QNetworkReply::NetworkError error = reply->error();
    if(!error){
        QByteArray array = reply->readAll();
        emit progress(FirmwareRequest::Progress_PARSING_RESP, 80);
        QJsonParseError err;
        doc = QJsonDocument::fromJson(array, &err);
        if(err.error != QJsonParseError::ParseError::NoError){
            emit done(err.errorString(), Result::Error, &doc);
            return;
        }
        if(doc.isObject())
        {
            QJsonObject obj = doc.object();
            QJsonObject::iterator status = obj.find("status");
            QJsonObject::iterator message = obj.find("message");
            QJsonObject::iterator files = obj.find("files");
            if(status == obj.end() || message == obj.end() || files == obj.end() || !files.value().isArray() || files.value().toArray().count() == 0)
            {
                emit done("Invalid response", Result::Error, &doc);
            }
            else if(status.value().toInt()!=200){
                 emit done(message.value().toString(), Result::Error, &doc);
            }
            emit done("File list loaded", Result::Success, &doc);
        }
        else{
            emit done(Response_INVALID_SERV, Result::Error, &doc);
        }

    }
    else{
        emit done(reply->errorString(), Result::Error, &doc);
    }
}

void FirmwareRequest::getFirmwareList(){
    emit progress(FirmwareRequest::Progress_CHECKIG, 10);
    QNetworkRequest request;
    QUrl url = QUrl(repositoryURL);
    QUrlQuery query;
    query.addQueryItem(queryUID, *uid);
    url.setQuery(query.query());
    request.setUrl(QUrl(repositoryURL));
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, clinetAgent);
    emit progress(FirmwareRequest::Progress_SENDING, 20);
    manager->get(request);
}
