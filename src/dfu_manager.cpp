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

#include <libusb-1.0/libusb.h>

#include "dfu/dfu.h"
#include "dfu/stm32mem.h"


DFUManager::DFUManager(QObject *parent) :
    QObject(parent)
{
    deviceName = new char[1024];
    handle = nullptr;
    timer = nullptr;
}

DFUManager::~DFUManager()
{
    if (handle) {
        qDebug() << "Releasing interface on delete.";
        libusb_release_interface(handle, interface);
        libusb_close(handle);
        handle = nullptr;
    }
}

void DFUManager::start()
{
    findIFace();
    if(!handle){
        timer = new QTimer(this);
        timer->setInterval(5000);
        connect(timer, SIGNAL(timeout()), this, SLOT(findIFace()));
        timer->start();
    }
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
    qDebug() << "Releasing Interface...";

    if (handle) {
        libusb_release_interface(handle, interface);
        libusb_close(handle);
        handle = nullptr;
    }
    qDebug() << "Trying to find DFU devices...";


    if(!findDev()) {
        qDebug() << "FATAL: No compatible device found!\n";
        emit lostDevice();
        return;
    }

    int state = dfu_getstate(handle, interface);

    if((state < 0) || (state == STATE_APP_IDLE)) {
        qDebug() << "Resetting device in firmware upgrade mode...";
        dfu_detach(handle, interface, 1000);
        libusb_release_interface(handle, interface);
        libusb_close(handle);
        handle = nullptr;
        emit lostDevice();
        return;
    }

    qDebug() << "Found DFU device " << deviceName;
    emit foundDevice();
}

bool DFUManager::findDev()
{
    extern libusb_context *usbcontext;
    libusb_device **list;
    libusb_device_handle *handle;
    libusb_device *device;
    ssize_t i,devicecount;
    devicecount = libusb_get_device_list( usbcontext, &list );

    for( i = 0; i < devicecount; i++ ) {
        device = list[i];
        struct libusb_device_descriptor descriptor;
        if( libusb_get_device_descriptor(device, &descriptor) ) {
            qDebug() <<  "Failed in libusb_get_device_descriptor";
            break;
        }

        sprintf(deviceName, "Device [%04X:%04X] ", descriptor.idVendor, descriptor.idProduct);
        if (descriptor.idVendor != 0x483 && descriptor.idVendor != 0x1d5) continue;
        if (descriptor.idProduct != 0xDF11) continue;
        qDebug() << "STM 32 Device found (F4)";

        int32_t interface = findDFUInterface(device, descriptor.bNumConfigurations);
        if(interface >= 0) {    /* The interface is valid. */
            if(libusb_open(device, &handle)==0) {
                if(libusb_set_configuration(handle, 1)==0) {
                    if(libusb_claim_interface(handle, interface)==0)
                    {

                                //dfu_makeidle(handle, static_cast<uint16_t>(interface))== 0
                        if(dfu_make_idle(handle, static_cast<uint16_t>(interface), 1)){
                            libusb_free_device_list( list, 1 );
                            this->dev = device;
                            this->interface = static_cast<uint16_t>(interface);
                            this->handle = handle;
                            return true;
                        }
                        libusb_free_device_list( list, 1 );
                        libusb_release_interface(handle, interface);

                    }
                }
                libusb_close(handle);
            }
        }
    }
    deviceName[0] = 0;
    return false;
}

int32_t DFUManager::findDFUInterface(struct libusb_device *device,  const uint8_t bNumConfigurations) {
    int32_t c,i,s;

    /* Loop through all of the configurations */
    for( c = 0; c < bNumConfigurations; c++ ) {
        struct libusb_config_descriptor *config;

        if( libusb_get_config_descriptor(device, c, &config) ) {
            qDebug() << "can't get_config_descriptor: %d\n";
            return -1;
        }

        qDebug() << QString("config %1: maxpower=%2*2 mA\n").arg(c).arg(config->MaxPower);

        /* Loop through all of the interfaces */
        for( i = 0; i < config->bNumInterfaces; i++ ) {
            struct libusb_interface interface;
            interface = config->interface[i];
            qDebug() << QString("interface %1").arg(i);
            /* Loop through all of the settings */
            for( s = 0; s < interface.num_altsetting; s++ ) {
                struct libusb_interface_descriptor setting;
                setting = interface.altsetting[s];
                qDebug() << QString("setting %1: class:%2, subclass %3, protocol:%3")
                            .arg(s).arg(setting.bInterfaceClass)
                            .arg(setting.bInterfaceSubClass)
                            .arg(setting.bInterfaceProtocol);

                /* Check if the interface is a DFU interface */
                if(USB_CLASS_APP_SPECIFIC == setting.bInterfaceClass && DFU_SUBCLASS == setting.bInterfaceSubClass)
                {
                    qDebug() << QString("Found DFU Interface: %1").arg(setting.bInterfaceNumber);
                    libusb_free_config_descriptor( config );
                    return setting.bInterfaceNumber;
                }
            }
        }

        libusb_free_config_descriptor( config );
    }

    return -1;
}


void DFUManager::flash(uint address, char* buffer, uint length)
{
    if(timer!=nullptr) timer->stop();
    address = 0x08000000;
    //tbd create copy
    int status;
    dfu_make_idle(handle, static_cast<uint16_t>(interface), 1);
    for (uint offset = 0U; offset < length; offset += block_size) {
        emit dfuProgress(address + offset, (offset*100)/length);
        status = stm32_mem_erase(handle, interface, address + offset);
        if (status != 0) {
            emit dfuDone(false, TEXT_ERROR_WHILE_CLEARING.arg(status));
            break;
        }
        status = stm32_mem_write(handle, interface, static_cast<void*>(buffer+offset), static_cast<int>(block_size));
        if (status != 0) {
            emit dfuDone(false, TEXT_ERROR_WHILE_WRITING.arg(status));
            break;
        }
    }



    stm32_mem_manifest(handle, interface);
    libusb_release_interface(handle, interface);
    libusb_close(handle);
    handle = nullptr;
    emit dfuDone(true, TEXT_ERROR_WRITE_OK);
    if(timer!=nullptr) timer->start();
}
