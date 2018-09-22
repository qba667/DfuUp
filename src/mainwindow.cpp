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

void MainWindow::onDone(const QString& message, FirmwareRequest::Result status,  QJsonDocument* result){
    ui->progressBar->setValue(100);
    appendStatus(message);
    switch(status){
    case FirmwareRequest::Success:
        fillFwList(result);
        setOperation(SelectFw);
        break;
    case FirmwareRequest::Error:
        setButton("Error", styleRed, &img_error, false);
        QTimer* timer = new QTimer;
        QObject::connect(timer, &QTimer::timeout, [this](){
            setOperation(GetFirmwareList);
        });
        connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
        timer->setSingleShot(true);
        timer->start(1000);
        break;
    }
}

void MainWindow::setOperation(Operation operation){
    currentOperation = operation;
    switch(operation){
        case GetFirmwareList:
            setButton("Check for updates", styleBlue, &img_reload, true);
            break;
        case SelectFw:
            setButton("Select resource", styleBlue, &img_reload, true);
            ui->fwList->setVisible(true);
            break;
        default:
            break;
    }
}
void MainWindow::fillFwList(QJsonDocument* result){
    QJsonObject obj = result->object();
    QJsonArray firmwares = obj["files"].toArray();
    QStringList items = QStringList();
    foreach (const QJsonValue & val, firmwares){
        items << ("[" + val["version"].toString() + "] " + val["name"].toString());
    }
    ui->fwList->addItems(items);

}
void MainWindow::getFwList(){
    QString uid = ui->uid1->text()+ui->uid2->text()+ui->uid3->text();
    setButton("Checking...", styleGreen, &img_wait, false);
    ui->progressBar->setVisible(true);
    fwRequest->setUID(uid);
    fwRequest->getFirmwareList();
}
void MainWindow::on_checkForUpdates_released()
{
    switch(currentOperation){
    case GetFirmwareList:
        getFwList();
        break;
    default:
        break;
    }

}

void MainWindow::on_fwList_currentIndexChanged(int index)
{

}
