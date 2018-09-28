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

#ifndef DFU_MANAGER_H
#define DFU_MANAGER_H

#include "dfu/dfu.h"

#include <QObject>
#include <QTimer>
#include <QMutex>

#define TRANSFER_SIZE (2048)

class DFUManager : public QObject
{
    Q_OBJECT
public:
    explicit DFUManager(QObject *parent = nullptr);
    ~DFUManager();
private:
    QTimer* detectionTimer;
    struct libusb_device *dev;
    dfu_device_t* dfu_device;
    uint8_t buffer[TRANSFER_SIZE];
signals:
    void foundDevice();
    void lostDevice();
    void dfuDone(const QString& message, bool success);
    void progress(const QString& message, int progress);
private slots:
    void onDetectionTick();
public slots:
    void start();
    void stop();
    void flash(uint address, char* buffer, uint length);

private:
    const QString TEXT_INVALID_PARAMS = "Invalid parameters!";
    const QString TEXT_CLEARING_STARTED = "Clearing chip...";
    const QString TEXT_CLEARING_PAGE_AT = "Clearing page at 0x%1...";
    const QString TEXT_CLEARING_DONE = "Clearing chip done.";
    const QString TEXT_ERROR_CLEARING = "An error occurred while clearing chip!";

    const QString TEXT_BURNING_STARTED = "Burning firmware...";
    const QString TEXT_BURNING_FW_PROGRESS = "Burning at 0x%1 - %2%";
    const QString TEXT_BURNING_ERROR = "An error [%1] occurred while writting!";
    const QString TEXT_BURNING_OK = "Flash programmed";
};

#endif // DFU_MANAGER_H
