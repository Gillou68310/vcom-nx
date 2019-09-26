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

#include "USBSerialDataInterface.h"
#include "USBSerialComInterface.h"

#define EP_IN 0
#define EP_OUT 1

USBSerialDataInterface::USBSerialDataInterface(int index) : USBInterface(index) {
}

USBSerialDataInterface::~USBSerialDataInterface() {
}

Result USBSerialDataInterface::initialize(const char* str)
{
    Result rc = usbAddStringDescriptor(&interface_descriptor.iInterface, "CDC ACM Data");
    if (R_SUCCEEDED(rc)) rc = usbInterfaceInit(getInterfaceIndex(), &interface_descriptor, NULL);
    if (R_SUCCEEDED(rc)) rc = usbAddEndpoint(getInterfaceIndex(), EP_IN, &endpoint_descriptor_in);
    if (R_SUCCEEDED(rc)) rc = usbAddEndpoint(getInterfaceIndex(), EP_OUT, &endpoint_descriptor_out);
    if (R_SUCCEEDED(rc)) rc = usbEnableInterface(getInterfaceIndex());
    return rc;
}

ssize_t USBSerialDataInterface::read(char *ptr, size_t len, u64 timestamp)
{
    return usbTransfer(getInterfaceIndex(), EP_OUT, UsbDirection_Read, (void*)ptr, len, timestamp);
}
ssize_t USBSerialDataInterface::write(const char *ptr, size_t len, u64 timestamp)
{
    return usbTransfer(getInterfaceIndex(), EP_IN, UsbDirection_Write, (void*)ptr, len, timestamp);
}