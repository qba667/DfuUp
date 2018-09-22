#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QSettings>
#include <string>
#include <QToolTip>
#include <QDateTime>
#include <QTimer>
#include "firmwarerequest.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum Operation {
        GetFirmwareList,
        SelectFw,
        DownloadFirmware,
        DownloadSD,
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
    void on_fwList_currentIndexChanged(int index);

private:
    void setOperation(Operation operation);
    void getFwList();
    void appendStatus(const QString& message);
    void checkUID();
    void setButton(const QString& message, const QString& style, QPixmap* icon, bool enabled);
    void fillFwList(QJsonDocument* result);

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
    Operation currentOperation = GetFirmwareList;
    QSettings settings;
    Ui::MainWindow *ui;
    QObject* lastMouseMoveObject;
    const QString UID_LIST = "UID_LIST";
};

#endif // MAINWINDOW_H
