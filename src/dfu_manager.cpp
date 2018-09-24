/*
 * This file is part of the Paparazzi UAV project.
 *
 * Copyright (C) 2012 Piotr Esden-Tempski <piotr@esden.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dfu_manager.h"

#include <QDebug>
#include <QFile>

#include "usb.h"
#include "dfu/dfu.h"
#include "dfu/stm32mem.h"

DFUManager::DFUManager(QObject *parent) :
    QObject(parent)
{
    usb_init();
    handle = nullptr;
    timer = nullptr;
    block_size = 1024;
    flash_size_mutex.lock();
    flash_size = 0x20000; /* 128kb */
    flash_size_mutex.unlock();
}

DFUManager::~DFUManager()
{
    if (handle) {
        qDebug() << "Releasing interface on delete.";
        usb_release_interface(handle, iface);
        usb_close(handle);
        handle = nullptr;
    }
}

void DFUManager::start()
{
    findIFace();
    timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer, SIGNAL(timeout()), this, SLOT(findIFace()));
    timer->start();
}
void DFUManager::stop()
{
    if(timer!=nullptr){
        timer->stop();
        timer = nullptr;
    }
}

void DFUManager::findIFace()
{
    QString *deviceName;

    qDebug() << "Releasing Interface...";

    if (handle) {
        usb_release_interface(handle, iface);
        usb_close(handle);
        handle = nullptr;
    }
    qDebug() << "Trying to find DFU devices...";


    if(!(dev = findDev())) {
        qDebug() << "FATAL: No compatible device found!\n";
        emit lostDevice();
        return;
    }

    int alternate = 0;
    handle = getDFUIface(dev, &iface, &alternate);
    if(!handle) {
        qDebug() << "FATAL: No compatible device found!\n";
        emit lostDevice();
        return;
    }

    //dfu_makeidle(handle, iface);

    state = dfu_getstate(handle, iface);
    if((state < 0) || (state == STATE_APP_IDLE)) {
        qDebug() << "Resetting device in firmware upgrade mode...";
        dfu_detach(handle, iface, 1000);
        usb_release_interface(handle, iface);
        usb_close(handle);
        handle = nullptr;
        emit lostDevice();
        return;
    }

    qDebug() << "Found device at " << dev->bus->dirname << ":" << dev->filename;

    deviceName = new QString("Found device at ");
    deviceName->append(dev->bus->dirname);
    deviceName->append(":");
    deviceName->append(dev->filename);

    emit foundDevice(deviceName);
}

struct usb_device *DFUManager::findDev()
{
    struct usb_bus *bus;
    struct usb_device *dev;
    struct usb_dev_handle *handle;
    char man[40];
    char prod[40];
    char buffer[512];
    usb_find_busses();
    usb_find_devices();

    for(bus = usb_get_busses(); bus; bus = bus->next) {
        for(dev = bus->devices; dev; dev = dev->next) {
            /* Check for ST Microelectronics vendor ID */
            if (dev->descriptor.idVendor != 0x483 && dev->descriptor.idVendor != 0x1d5) continue;
            handle = usb_open(dev);
            usb_get_string_simple(handle, dev->descriptor.iManufacturer, man, sizeof(man));
            usb_get_string_simple(handle, dev->descriptor.iProduct, prod, sizeof(prod));

#if 1
            sprintf(buffer, "%s:%s [%04X:%04X] %s : %s\n", bus->dirname, dev->filename, dev->descriptor.idVendor, dev->descriptor.idProduct, man, prod);
            qDebug() << buffer;
#endif
            usb_close(handle);
            if (dev->descriptor.idProduct == 0xDF11) {
                block_size = 1024;
                flash_size_mutex.lock();
                flash_size = 0x200000; /* 2MB */
                flash_size_mutex.unlock();
                return dev;
            }
        }
    }
    return nullptr;
}

struct usb_dev_handle *DFUManager::getDFUIface(struct usb_device *dev, uint16_t *interface, int* alternate)
{
    int i, j, k;
    struct usb_config_descriptor *config;
    struct usb_interface_descriptor *iface;

    usb_dev_handle *handle;
    int error = 0;
    for(i = 0; i < dev->descriptor.bNumConfigurations; i++) {
        config = &dev->config[i];

        for(j = 0; j < config->bNumInterfaces; j++) {
            for(k = *alternate; k < config->interface[j].num_altsetting; k++) {
                iface = &config->interface[j].altsetting[k];
                //this will get first valid interface
                if((iface->bInterfaceClass == 0xFE) && (iface->bInterfaceSubClass = 0x01)) {
                    *interface = iface->bInterfaceNumber;
                    *alternate = config->interface[j].num_altsetting;
                    for(int attempt = 0; attempt <3; attempt++){
                        handle = usb_open(dev);
    #if 0
                        qDebug() << QString("Setting Configuration %1...").arg(i);
                        error = usb_set_configuration(handle, i);
                        if (error < 0) qDebug() << QString("Error %1...").arg(error);
    #endif
                        qDebug() << QString("Claiming USB DFU Interface %1...").arg(j);
                        error = usb_claim_interface(handle, j);
                        if (error < 0) qDebug() << QString("Error %1...").arg(error);
                        qDebug() << QString("Setting Alternate Setting  %1...").arg(k);
                        error = usb_set_altinterface(handle, k);
                        if (error < 0) qDebug() << QString("Error %1...").arg(error);
                        qDebug() << "Determining device status: ";
                        dfu_status status;
                        error = dfu_getstatus(handle, *interface, &status);
                        if (error < 0) qDebug() << QString("Error %1...").arg(error);
                        qDebug() << QString("Device state %1 status %2").arg(status.bState).arg(status.bStatus);
                        qDebug() << QString("Waiting %1...").arg(status.bwPollTimeout);
                        if(status.bwPollTimeout > 500) status.bwPollTimeout = 500;
                        Sleep(status.bwPollTimeout);

                        switch (status.bState) {
                            case STATE_APP_IDLE:
                            case STATE_APP_DETACH:
                                qDebug() <<"Device still in Runtime Mode!";
                                break;
                            case STATE_DFU_ERROR:
                                qDebug() << "dfuERROR, clearing status";
                                if(dfu_clrstatus(handle, *interface)< 0)   qDebug() <<"error clear_status";
                                usb_close(handle);
                                continue;
                            case STATE_DFU_DOWNLOAD_IDLE:
                            case STATE_DFU_UPLOAD_IDLE:
                                qDebug() << "aborting previous incomplete transfer\n";
                                if(dfu_abort(handle, *interface) <0) qDebug() << "can't send DFU_ABORT";
                                usb_close(handle);
                                continue;
                            case STATE_DFU_IDLE:
                                qDebug() << "dfuIDLE, continuing\n";
                                return handle;
                            default:
                                break;
                            }

                            if (DFU_STATUS_OK != status.bStatus ) {
                                qDebug() << QString("WARNING: DFU Status: '%1'").arg(status.bStatus);
                                /* Clear our status & try again. */
                                if(dfu_clrstatus(handle, *interface) < 0)
                                    qDebug() << "USB communication error";

                                if(dfu_getstatus(handle, *interface, &status))
                                    qDebug() << "USB communication error";

                                if (DFU_STATUS_OK != status.bStatus) qDebug() << QString("Status is not OK: %1").arg(status.bStatus);
                                Sleep(status.bwPollTimeout);
                            }

                            usb_close(handle);
                    }

                }
            }
        }
    }
    return nullptr;
}

void DFUManager::flash(uint address, char* buffer, uint length)
{
    if(timer!=nullptr) timer->stop();
    //tbd create copy
    dfu_makeidle(handle, iface);
    for (uint offset = 0U; offset < length; offset += block_size) {
        emit dfuProgress(address + offset, (offset*100)/length);
        if (stm32_mem_erase(handle, iface, address + offset) != 0) {
            emit dfuDone(false, TEXT_ERROR_WHILE_CLEARING);
            if(timer!=nullptr) timer->start();
        }
        stm32_mem_write(handle, iface, static_cast<void*>(buffer+offset), static_cast<int>(block_size));
    }

    stm32_mem_manifest(handle, iface);
    usb_release_interface(handle, iface);
    usb_close(handle);
    handle = nullptr;
    emit dfuDone(true, TEXT_ERROR_WRITE_OK);
    if(timer!=nullptr) timer->start();
}

uint DFUManager::get_flash_size()
{
    uint size;
    flash_size_mutex.lock();
    size = flash_size;
    flash_size_mutex.unlock();
    return size;
}
