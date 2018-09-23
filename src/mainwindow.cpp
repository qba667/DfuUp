#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "firmwarerequest.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QStringList list = QStringList();
    list.append("");
    list.append("");
    list.append("");
    list = settings.value(UID_LIST, QVariant(list)).toStringList();
    ui->uid1->setText(list.at(0));
    ui->uid2->setText(list.at(1));
    ui->uid3->setText(list.at(2));
    ui->fwList->setVisible(false);
    appendStatus("NV14-Update tool started");
    qApp->installEventFilter(this);
    fwRequest = new FirmwareRequest();
    connect(fwRequest, SIGNAL (progress(const QString&, int)), this, SLOT (onProgress(const QString&, int)));
    connect(fwRequest, SIGNAL (done(const QString&, FirmwareRequest::Result, QJsonDocument*)), this, SLOT (onDone(const QString&, FirmwareRequest::Result, QJsonDocument*)));
    connect(fwRequest, SIGNAL (done(const QString&, FirmwareRequest::Result, char*, int)), this, SLOT (onDone(const QString&, FirmwareRequest::Result, char*, int)));
}

void MainWindow::appendStatus(const QString& message){
 ui->fwInfo->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] " + message);
}

MainWindow::~MainWindow()
{
    delete ui;
}
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == ui->uid1 || obj == ui->uid2 || obj == ui->uid3) && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
        int key = keyEvent->key();
        if(key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9){
            keyEvent->accept();
            return false;
        }
        if(key >= Qt::Key::Key_A && key <= Qt::Key::Key_F){
            keyEvent->accept();
            return false;
        }
        if(key >= Qt::Key::Key_Escape && key <= Qt::Key::Key_Control){
            keyEvent->accept();
            return false;
        }
        return true;

    }
    if(event->type() == QEvent::MouseMove){
        if(obj == ui->labelHelp){
            if(lastMouseMoveObject != ui->labelHelp) {
                lastMouseMoveObject = ui->labelHelp;
                QTimer* timer = new QTimer;
                QObject::connect(timer, &QTimer::timeout, [this](){
                    if(!QToolTip::isVisible() && lastMouseMoveObject == ui->labelHelp){
                         QToolTip::showText(QCursor::pos(), "<img src=':/resources/CPU_ID.png'>", this, QRect(), 5000);
                    }

                });
                connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
                timer->setSingleShot(true);
                timer->start(1000);
            }
        }
        else{
            lastMouseMoveObject = nullptr;
        }

    }

    return QObject::eventFilter(obj, event);
}


void MainWindow::checkUID(){
    if(ui->uid1->text().length() == ui->uid1->maxLength() &&
            ui->uid2->text().length() == ui->uid2->maxLength() &&
            ui->uid3->text().length() == ui->uid3->maxLength()){
        QStringList list = QStringList();
        list.append(ui->uid1->text());
        list.append(ui->uid2->text());
        list.append(ui->uid3->text());
        settings.setValue(UID_LIST, QVariant(list));
        ui->checkForUpdates->setEnabled(true);
    }
}

void MainWindow::on_uid1_textChanged(const QString &arg1)
{
    ui->uid1->setText(arg1.toUpper());
    if(arg1.length() == ui->uid1->maxLength()) ui->uid2->setFocus();
    checkUID();
}

void MainWindow::on_uid2_textChanged(const QString &arg1)
{
    ui->uid2->setText(arg1.toUpper());
    if(arg1.length() == ui->uid2->maxLength()) ui->uid3->setFocus();
    checkUID();
}

void MainWindow::on_uid3_textChanged(const QString &arg1)
{
    ui->uid3->setText(arg1.toUpper());
    checkUID();
}

void MainWindow::onProgress(const QString& message, int progress){
    ui->progressBar->setValue(progress);
    appendStatus(message);
}

void MainWindow::setButton(const QString& message, const QString& style, QPixmap* icon, bool enabled){
    ui->checkForUpdates->setIcon(*icon);
    ui->checkForUpdates->setText(message);
    ui->checkForUpdates->setStyleSheet(style);
    ui->checkForUpdates->setEnabled(enabled);
}

void MainWindow::setError(Operation nextOperation){
    setButton("Error", styleRed, &img_error, false);
    QTimer* timer = new QTimer;
    QObject::connect(timer, &QTimer::timeout, [this, nextOperation](){
        setOperation(nextOperation);
    });
    connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
    timer->setSingleShot(true);
    timer->start(5000);
}

void MainWindow::onDone(const QString& message, FirmwareRequest::Result status, char* result, int length){
    ui->progressBar->setValue(100);
    appendStatus(message);
    if(status == FirmwareRequest::Error){
        setError(SelectResource);
        return;
    }
    RemoteFileInfo rfi;
    rfi.parse(ui->fwList->currentData().toJsonValue());
    appendStatus(message);
    appendStatus(Text_VALIDATIN_CHECKSUM);
    if(!rfi.isValid(result, length)){
       appendStatus(Text_INVALID_CHECKSUM);
       setError(SelectResource);
       return;
    }
    switch(rfi.type){
    case RemoteFileInfo::ResourceType::Archive:
        setOperation(SelectResource);
        ui->fwList->setCurrentIndex(0);
        break;
    case RemoteFileInfo::ResourceType::Firmware:
        setOperation(UpdateTX);
        break;
    }
}
void MainWindow::onDone(const QString& message, FirmwareRequest::Result status,  QJsonDocument* result){
    ui->progressBar->setValue(100);
    appendStatus(message);
    if(status == FirmwareRequest::Error){
        setError(GetResourceList);
        return;
    }
    fillFwList(result);
    setOperation(SelectResource);
}

void MainWindow::setOperation(Operation operation){
    currentOperation = operation;
    switch(operation){
    case GetResourceList:
        setButton(Text_CHECK_UPDATES, styleBlue, &img_reload, true);
        break;
    case SelectResource:
        setButton(Text_SELECT_RESOURCE, styleBlue, &img_reload, true);
        ui->fwList->setVisible(true);
        break;
    case DownloadFirmware:
        setButton(Text_DOWNLOAD_FW, styleBlue, &img_reload, true);
        ui->fwList->setVisible(true);
        break;
    case DownloadArchive:
        setButton(Text_DOWNLOAD_ARCHIVE, styleBlue, &img_reload, true);
        ui->fwList->setVisible(true);
        break;
    case UpdateTX:
        setButton(Text_UPDATE_TX, styleBlue, &img_reload, true);
        ui->fwList->setVisible(true);
        break;
    case DetectTX:
        setButton(Text_DETECTING_TX, styleBlue, &img_wait, false);
        ui->fwList->setVisible(false);
        break;
    case BurnFirmware:
        setButton(Text_UPDATING_TX, styleBlue, &img_wait, false);
        ui->fwList->setVisible(false);
        break;
    default:
        break;
    }
}

void MainWindow::fillFwList(QJsonDocument* result){
    QJsonObject obj = result->object();
    QJsonArray firmwares = obj["files"].toArray();
    ui->fwList->addItem("--------------");
    foreach (const QJsonValue & val, firmwares){
        ui->fwList->addItem("[" + val["version"].toString() + "] " + val["name"].toString(), QVariant(val));
    }
}
void MainWindow::getFwList(){
    QString uid = ui->uid1->text()+ui->uid2->text()+ui->uid3->text();
    setButton("Checking...", styleGreen, &img_wait, false);
    fwRequest->setUID(uid);
    fwRequest->getResourceList();
}
void MainWindow::on_checkForUpdates_released()
{
    switch(currentOperation){
    case GetResourceList:
        getFwList();
        break;
    case DownloadArchive: {
        RemoteFileInfo rfi;
        rfi.parse(ui->fwList->currentData().toJsonValue());
        QString defaultFilter("%1 files (*.%1)");
        defaultFilter = defaultFilter.arg(rfi.fileName.split(".").last());
        QString path = QFileDialog::getSaveFileName(this, tr("Save file as"), rfi.fileName, defaultFilter + ";;All files (*.*)", &defaultFilter);
        if(path.length()==0){
            return;
        }
        setButton(Text_DOWNLOADING, styleGreen, &img_wait, false);
        fwRequest->getResource(rfi.url, path);
    }
        break;
    case DownloadFirmware:
        {
            RemoteFileInfo rfi;
            rfi.parse(ui->fwList->currentData().toJsonValue());
            setButton(Text_DOWNLOADING, styleGreen, &img_wait, false);
            fwRequest->getResource(rfi.url);
        }
        break;
    case UpdateTX:
    {
        setOperation(DetectTX);
    }
        break;
    default:
        break;
    }
}

void MainWindow::on_fwList_currentIndexChanged(int index)
{
     QVariant data = ui->fwList->itemData(index);
     if(data == NULL) return;
    RemoteFileInfo rfi;
    rfi.parse(data.toJsonValue());

    switch(rfi.type){
        case RemoteFileInfo::Archive:
            setOperation(DownloadArchive);
            break;
        case RemoteFileInfo::Firmware:
            setOperation(DownloadFirmware);
            break;
    }
}
