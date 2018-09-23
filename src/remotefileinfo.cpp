#include "remotefileinfo.h"
#include <QCryptographicHash>

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
    if(it != obj.end()) fileName = it.value().toString();
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
