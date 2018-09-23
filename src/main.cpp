#include "mainwindow.h"
#include <QApplication>
extern "C"
{
    #include <libusb-1.0/libusb.h>
}
int main(int argc, char *argv[])
{
    libusb_init(NULL);
    QCoreApplication::setOrganizationName("UndergroundFPV");
    QCoreApplication::setOrganizationDomain("undergroundfpv.com");
    QCoreApplication::setApplicationName("NV14 Updater");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    int result = a.exec();
    libusb_exit(NULL);
    return result;
}
