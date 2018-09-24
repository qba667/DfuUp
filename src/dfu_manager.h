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


class DFUManager : public QObject
{
    Q_OBJECT
public:
    explicit DFUManager(QObject *parent = nullptr);
    ~DFUManager();
    uint get_flash_size();

private:
    QTimer* timer;
    struct usb_device *dev;
    struct usb_dev_handle *handle;
    uint16_t iface;
    int state;
    uint block_size;
    QMutex flash_size_mutex;
    uint flash_size;

    struct usb_device *findDev(void);
    struct usb_dev_handle *getDFUIface(struct usb_device *dev, uint16_t *interface, int* alternate);

signals:
    void foundDevice(QString *string);
    void lostDevice();

    void dfuDone(bool success, const QString& message);
    void dfuProgress(uint address, uint percentage);
    
private slots:
    void findIFace();//on timer

public slots:
    void start();
    void stop();
    void flash(uint address, char* buffer, uint length);

public slots:

private:
    const QString TEXT_ERROR_WHILE_CLEARING = "Error clearing chip";
    const QString TEXT_ERROR_WRITE_OK = "Flash programmed";
};

#endif // DFU_MANAGER_H
