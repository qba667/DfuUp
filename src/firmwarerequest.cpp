#include "firmwarerequest.h"

FirmwareRequest::FirmwareRequest()
{
    lastProgress = -1;
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
    lastProgress = -1;
    if(reply != nullptr) {
        delete reply;
        reply = nullptr;
    }
    if(targetFile != nullptr) {
        if(targetFile->isOpen()) {
            targetFile->flush();
            targetFile->close();
        }
        delete targetFile;
        targetFile = nullptr;
    }
    if(buffer !=nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }
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
    QNetworkReply::NetworkError error = reply->error();
    int contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toInt();
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray resp = reply->readAll();
    if(!error && code >=200 && code <=300) emit done(TEXT_RESPNSE_RECIEVED,  Result::Success, buffer, contentLength);
    else emit done(reply->errorString(), Result::Error, buffer, contentLength);
}


void FirmwareRequest::onReadyRead(){
    int contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toInt();
    if(buffer == nullptr){
        buffer = new char[contentLength];
        currentPos = buffer;
    }
    qint64 count = 0;
    do{
        //check overflow
        int freeSpace = contentLength - static_cast<int>(currentPos - buffer);
        if(freeSpace > 1024) freeSpace = 1024;
        count = reply->read(currentPos, freeSpace);
        if(targetFile!=nullptr && targetFile->isOpen() && count > 0) targetFile->write(currentPos, count);
        currentPos += count;
        int compleated = static_cast<int>(currentPos - buffer);
        int progressVal = (compleated*100)/ contentLength;
        if(lastProgress!=progressVal){
           lastProgress = progressVal;
           qDebug() << "compleated: " << compleated << "progress: " << progressVal << "contentLength " << contentLength ;
           emit progress(TEXT_PROGRESS_DOWNLOADING.arg(progressVal), progressVal);
        }

    } while(count > 0);
}

void FirmwareRequest::onFinished(){
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
    connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

void FirmwareRequest::getResourceList(){
    emit progress(FirmwareRequest::TEXT_CHECKING_4_UPDATES, 10);
    getResource(QUrl(repositoryURL));
    emit progress(FirmwareRequest::TEXT_SENDING_REQUEST, 20);
}
