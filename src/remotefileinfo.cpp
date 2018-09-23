#include "remotefileinfo.h"
#include <QCryptographicHash>

const QString RemoteFileInfo::TEXT_invalidResponse = "Invalid response";

RemoteFileInfo::RemoteFileInfo()
{

}

void RemoteFileInfo::parse(QJsonValue value){
    QJsonObject obj = value.toObject();
    QJsonObject::iterator it;

    it = obj.find(JSON_id);
    if(it != obj.end()) id = it.value().toString();

    it = obj.find(JSON_type);
    if(it != obj.end()) type = it.value() == "firmware" ? Firmware : Archive;

    it = obj.find(JSON_name);
    if(it != obj.end()) name = it.value().toString();
    it = obj.find(JSON_fileName);
    if(it != obj.end())fileName = it.value().toString();
    it = obj.find(JSON_version);
    if(it != obj.end()) version = it.value().toString();
    it = obj.find(JSON_sha1sum);
    if(it != obj.end()) sha1sum = it.value().toString();
    it = obj.find(JSON_startAddress);
    if(it != obj.end()) startAddress = it.value().toInt();
    it = obj.find(JSON_vid);
    if(it != obj.end()) vid = it.value().toString();
    it = obj.find(JSON_pid);
    if(it != obj.end()) pid = it.value().toString();
    it = obj.find(JSON_url);
    if(it != obj.end()) url = QUrl(it.value().toString());

}
bool RemoteFileInfo::isValid(char* data, int length){
    QCryptographicHash sh1(QCryptographicHash::Algorithm::Sha1);
    sh1.addData(data, length);
    return QString(sh1.result().toHex()).compare(sha1sum, Qt::CaseSensitivity::CaseInsensitive) == 0;
}

bool RemoteFileInfo::ParseResourceList(char* data, int length, std::vector<RemoteFileInfo>& results, QString& error){
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(data, length), &err);
    if(err.error != QJsonParseError::ParseError::NoError){
        error = err.errorString();
        return false;
    }
    if(doc.isObject())
    {
        QJsonObject obj = doc.object();
        QJsonObject::iterator status = obj.find("status");
        QJsonObject::iterator message = obj.find("message");
        QJsonObject::iterator files = obj.find("files");

        if(status == obj.end() || message == obj.end() || files == obj.end() || !files.value().isArray() || files.value().toArray().count() == 0)
        {
            error = TEXT_invalidResponse;
            return false;
        }
        else if(status.value().toInt()!=200){
            error = message.value().toString();
            return false;
        }

        foreach (const QJsonValue & value, files->toArray()){
            RemoteFileInfo rfi;
            rfi.parse(value);
            results.push_back(rfi);
        }
        return true;
    }
    error = TEXT_invalidResponse;
    return false;
}
