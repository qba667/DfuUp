#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QSettings>
#include <string>
#include <QToolTip>
#include <QDateTime>
#include <QTimer>
#include <QFileDialog>
#include "firmwarerequest.h"
#include "remotefileinfo.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum Operation {
        GetResourceList,
        SelectResource,
        DownloadFirmware,
        DownloadArchive,
        UpdateTX,
        DetectTX,
        BurnFirmware,
        None
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
private slots:
    void on_uid1_textChanged(const QString &arg1);

    void on_uid2_textChanged(const QString &arg1);

    void on_uid3_textChanged(const QString &arg1);

    void on_checkForUpdates_released();
    void onProgress(const QString& message, int progress);
    void onDone(const QString& message, FirmwareRequest::Result status, QJsonDocument* result);
    void onDone(const QString& message, FirmwareRequest::Result status, char* result, int length);
    void on_fwList_currentIndexChanged(int index);
private:
    void setError(Operation nextOperation);
    void setOperation(Operation operation);
    void getFwList();
    void appendStatus(const QString& message);
    void checkUID();
    void setButton(const QString& message, const QString& style, QPixmap* icon, bool enabled);
    void fillFwList(QJsonDocument* result);
    QVariant GetFwInfo(QJsonValue& val);
    QPixmap img_error = QPixmap(QString::fromUtf8(":/resources/error.png"));
    QPixmap img_ok = QPixmap(QString::fromUtf8(":/resources/ok.png"));
    QPixmap img_help = QPixmap(QString::fromUtf8(":/resources/help.png"));
    QPixmap img_info = QPixmap(QString::fromUtf8(":/resources/info.png"));
    QPixmap img_reload = QPixmap(QString::fromUtf8(":/resources/reload.png"));
    QPixmap img_sd = QPixmap(QString::fromUtf8(":/resources/sd.png"));
    QPixmap img_wait = QPixmap(QString::fromUtf8(":/resources/wait.png"));
    QPixmap img_warning = QPixmap(QString::fromUtf8(":/resources/warning.png"));

    QString styleRed = "background-color: rgb(189, 33, 48); color: rgb(255, 255, 255); font: 75 12pt \"Courier New\"";
    QString styleGreen = "background-color: rgb(30, 126, 52); color: rgb(255, 255, 255); font: 75 12pt \"Courier New\"";
    QString styleOrange = "background-color: rgb(255, 193, 7); color: rgb(255, 255, 255); font: 75 12pt \"Courier New\"";
    QString styleBlue = "background-color: rgb(0, 88, 204); color: rgb(255, 255, 255);\nfont: 75 12pt \"Courier New\"";
    FirmwareRequest* fwRequest;
    Operation currentOperation = GetResourceList;
    QSettings settings;
    Ui::MainWindow *ui;
    QObject* lastMouseMoveObject;
    const QString UID_LIST = "UID_LIST";
    const QString Text_VALIDATIN_CHECKSUM = "Validating checksum";
    const QString Text_INVALID_CHECKSUM = "Invalid checksm";
    const QString Text_CHECK_UPDATES = "Check for updates";
    const QString Text_SELECT_RESOURCE = "Select resrouce";
    const QString Text_DOWNLOAD_ARCHIVE = "Download archive";
    const QString Text_DOWNLOAD_FW = "Download firmware";
    const QString Text_UPDATE_TX = "Update TX";
    const QString Text_UPDATING_TX = "TX updating...";
    const QString Text_DETECTING_TX = "TX detecting...";
    const QString Text_DOWNLOADING = "Downloading...";
};

#endif // MAINWINDOW_H
