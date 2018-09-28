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

    detectionTimer = new QTimer(this);
    detectionTimer->setInterval(5000);
    connect(detectionTimer, SIGNAL(timeout()), this, SLOT(onDetectionTick()));

    //deviceName = new char[1024];
    dfu_device = new dfu_device_t();
    dfu_device->handle = nullptr;
    dfu_device->interface = 0;
}

DFUManager::~DFUManager()
{
    if (dfu_device->handle) {
        qDebug() << "Releasing interface on delete.";
        libusb_release_interface(dfu_device->handle, dfu_device->interface);
        libusb_close(dfu_device->handle);
        dfu_device->handle = nullptr;
    }
    detectionTimer->stop();
}

void DFUManager::start()
{
    onDetectionTick();
    if(!dfu_device->handle){
        detectionTimer->start();
    }
}
void DFUManager::stop()
{
    detectionTimer->stop();
}

void DFUManager::onDetectionTick()
{
     if (dfu_device->handle) {
         detectionTimer->stop();
         return;
     }

    qDebug() << "Releasing Interface...";

    if (dfu_device->handle) {
        libusb_release_interface(dfu_device->handle, dfu_device->interface);
        libusb_close(dfu_device->handle);
        dfu_device->handle = nullptr;
    }
    qDebug() << "Trying to find DFU devices...";

    this->dev = dfu_device_init(0x483, 0xDF11, 0,0, dfu_device, 1, 1);
    if(this->dev == nullptr){
        qDebug() << "FATAL: No compatible device found!\n";
        emit lostDevice();
        return;
    }

    int state = dfu_get_state(dfu_device);

    if((state < 0) || (state == STATE_APP_IDLE)) {
        qDebug() << "Resetting device in firmware upgrade mode...";
        dfu_detach(dfu_device, 1000);
        libusb_release_interface(dfu_device->handle, dfu_device->interface);
        libusb_close(dfu_device->handle);
        dfu_device->handle = nullptr;
        emit lostDevice();
        return;
    }

    qDebug() << "Found DFU device ";
    emit foundDevice();
}

void DFUManager::flash(uint startAddress, char* data, uint length)
{
    detectionTimer->stop();
    int32_t status = UNSPECIFIED_ERROR;
    if(startAddress < 0x08000000 || length > 0x00200000) {
      emit dfuDone(TEXT_INVALID_PARAMS, false);
      return;
    }
    uint endAddress = startAddress + length;
    uint numberOfPages = sizeof(stm32_sector_addresses)-1;

    int firstPage = -1;
    uint lastPage = 1;
    //find pages
    for(uint page = 0; page < numberOfPages ; page++){
        uint pageStart = stm32_sector_addresses[page];
        uint pageEnd =  stm32_sector_addresses[page+1];

        if(pageStart > endAddress) break; //page start after image
        if(pageEnd < startAddress) continue; //page ends before image
        if(firstPage == -1) firstPage = static_cast<int>(page);
        lastPage = page;
    }

    if(firstPage == -1){
        emit dfuDone(TEXT_INVALID_PARAMS, false);
        return;
    }
    emit progress(TEXT_CLEARING_STARTED, 0);
    for(uint page = static_cast<uint>(firstPage); page <= lastPage ; page++){
        emit progress(TEXT_CLEARING_PAGE_AT.arg(stm32_sector_addresses[page], 8, 16, QLatin1Char('0')), 0);
        if((status = stm32_page_erase(dfu_device, stm32_sector_addresses[page], false))) {
            emit dfuDone(TEXT_ERROR_CLEARING, false);
            return;
        }
    }
    emit progress(TEXT_CLEARING_DONE, 0);
    emit progress(TEXT_BURNING_STARTED, 0);

    uint32_t offset = 0;
    uint32_t bytes;


    if((status = stm32_set_address_ptr(dfu_device, startAddress))) {
        emit dfuDone(TEXT_BURNING_ERROR.arg(status), false);
        return;
    }

    dfu_set_transaction_num( 2 ); /* sets block offset 0 */


    while( offset < length ) {
        memset(buffer, 0xFF, TRANSFER_SIZE);
        bytes = length - offset;
        if(bytes > TRANSFER_SIZE) bytes = TRANSFER_SIZE;
        memcpy(buffer, data + offset, bytes);
        offset += TRANSFER_SIZE;
        if( (status = stm32_write_block( dfu_device, TRANSFER_SIZE, buffer )) ) {
            emit dfuDone(TEXT_BURNING_ERROR.arg(status), false);
            return;
        }
        int p = static_cast<int>(((offset*100U)/length));
        emit progress(TEXT_BURNING_FW_PROGRESS.arg(startAddress + offset, 8, 16, QLatin1Char('0')).arg(p), p);
      }

    stm32_start_app(dfu_device, false);

    dfuDone(TEXT_BURNING_OK, true);

    libusb_release_interface(dfu_device->handle, dfu_device->interface);
    libusb_close(dfu_device->handle);
    dfu_device->handle = nullptr;
    emit lostDevice();
}
