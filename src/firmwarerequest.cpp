#include "firmwarerequest.h"

FirmwareRequest::FirmwareRequest()
{
    this->manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(managerFinished(QNetworkReply*)));
    connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),this,SLOT(onIgnoreSSLErrors(QNetworkReply*, const QList<QSslError>&)));
}

FirmwareRequest::~FirmwareRequest()
{
    delete uid;
    delete manager;
    cleanup();
}
void FirmwareRequest::cleanup(){
    if(reply != nullptr) delete reply;
    if(targetFile != nullptr) {
        if(targetFile->isOpen()) {
            targetFile->flush();
            targetFile->close();
        }
        delete targetFile;
    }
    if(buffer !=nullptr) delete[] buffer;
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
    emit progress(FirmwareRequest::TEXT_RECIEVING_RESPONSE, 60);
    QNetworkReply::NetworkError error = reply->error();
    if(!error){
        QByteArray array = reply->readAll();
        emit progress(FirmwareRequest::TEXT_PARSING_RESPONSE, 80);
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
            emit done(TEXT_INVALID_RESPNSE, Result::Error, &doc);
        }

    }
    else{
        emit done(reply->errorString(), Result::Error, &doc);
    }
}


void FirmwareRequest::onReadyRead(){
    int contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toInt();
    if(buffer == nullptr){
        buffer = new char[contentLength];
        currentPos = buffer;
    }
    qint64 count = 0;
    int freeSpace = 0;
    do{
        //check overflow
        int freeSpace = contentLength - (currentPos - buffer);
        if(freeSpace > 1024) freeSpace = 1024;
        count = reply->read(currentPos, freeSpace);
        if(targetFile!=nullptr) targetFile->write(currentPos, count);
        currentPos += count;
    } while(count > 0 && freeSpace > 0);
    int pVal = ((currentPos - buffer)*100)/contentLength;
    emit progress(TEXT_PROGRESS_DOWNLOADING.arg(pVal), pVal);
}


void FirmwareRequest::onFinished(){
    progress(TEXT_PROGRESS_DOWNLOADING.arg(100), 100);

    if(targetFile!=nullptr) {
        targetFile->flush();
        targetFile->close();
    }
}



void FirmwareRequest::getResource(QUrl url, QString file){
    cleanup();
    QNetworkRequest request;
    QUrlQuery query;
    query.addQueryItem(queryUID, *uid);
    url.setQuery(query.query());
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, clinetAgent);
    if(file.count() > 0) {
        targetFile = new QFile(file);
        try {
             targetFile->open(QFile::OpenModeFlag::ReadWrite);
        } catch (...) {
            emit done(TEXT_CAN_NOT_CREATE_FILE, Error, buffer, 0);
            return;
        }
    }

    reply = manager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(onFinished));
    connect(reply, SIGNAL(readyRead()), this, SLOT(onReadyRead));
}

void FirmwareRequest::getResourceList(){
    emit progress(FirmwareRequest::TEXT_CHECKING_4_UPDATES, 10);
    getResource(QUrl(repositoryURL));
    emit progress(FirmwareRequest::TEXT_SENDING_REQUEST, 20);
}
