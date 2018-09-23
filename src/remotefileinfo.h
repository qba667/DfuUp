#ifndef REMOTEFILEINFO_H
#define REMOTEFILEINFO_H

#include <QJsonValue>
#include <QJsonObject>
#include <QUrl>
class RemoteFileInfo
{
public:
    enum ResourceType {
        Firmware,
        Archive
    };

    RemoteFileInfo();
    void parse(QJsonValue value);
    bool isValid(char* data, int length);

    QString id;
    ResourceType type;
    QString name;
    QString fileName;
    QString version;
    QString sha1sum;
    int startAddress;
    QString vid;
    QString pid;
    QUrl url;

    const QString JSON_id = "id";
    const QString JSON_type = "type";
    const QString JSON_name = "name";
    const QString JSON_fileName = "fileName";
    const QString JSON_version = "version";
    const QString JSON_sha1sum = "sha1sum";
    const QString JSON_startAddress = "startAddress";
    const QString JSON_vid = "vid";
    const QString JSON_pid = "pid";
    const QString JSON_url = "url";
};

#endif // REMOTEFILEINFO_H
