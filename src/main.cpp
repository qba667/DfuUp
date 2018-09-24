#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{

    QCoreApplication::setOrganizationName("UndergroundFPV");
    QCoreApplication::setOrganizationDomain("undergroundfpv.com");
    QCoreApplication::setApplicationName("NV14 Updater");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
