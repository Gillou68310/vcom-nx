/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __USB_SERIAL_H
#define __USB_SERIAL_H

#include "USBSerialComInterface.h"
#include "USBSerialDataInterface.h"

class USBSerial {
private:
    static int interface_count;
    int index;
    USBSerialComInterface *com;
    USBSerialDataInterface *data;
public:
            USBSerial();
    virtual ~USBSerial();

    Result initialize(const char* str);
    ssize_t read(char *ptr, size_t len);
    ssize_t write(const char *ptr, size_t len);
    ssize_t sendEvent(const char *ptr, size_t len);
    int getIndex() {return index;}
};

#endif /* __USB_SERIAL_H */
