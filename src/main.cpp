#include "mainwindow.h"
#include <QApplication>

#if defined(__cplusplus)
extern "C" {
#include <libusb-1.0/libusb.h>
libusb_context *usbcontext;
}
#endif



int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("UndergroundFPV");
    QCoreApplication::setOrganizationDomain("undergroundfpv.com");
    QCoreApplication::setApplicationName("NV14 Updater");
    QApplication a(argc, argv);

    if (libusb_init(&usbcontext)) {
        qDebug() << "can't init libusb.\n";
    }
    libusb_set_debug(usbcontext, 3);

    MainWindow w;
    //
    w.show();
    int resut = a.exec();
    libusb_exit(usbcontext);
    return resut;
}
