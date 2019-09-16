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

#include <malloc.h>
#include "USBCDCDataInterface.h"
#include "USBCDCComInterface.h"

void dump(void* buffer, int size, Log::Logger *logger);

#define EP_IN 0
#define EP_OUT 1

USBCDCDataInterface::USBCDCDataInterface(int index) : USBInterface(index) {
}

USBCDCDataInterface::~USBCDCDataInterface() {
}

Result USBCDCDataInterface::initialize()
{
    Result rc = usbAddStringDescriptor(&interface_descriptor.iInterface, "CDC ACM Data");
    if (R_SUCCEEDED(rc)) rc = usbInterfaceInit(interface_index, &interface_descriptor, NULL);
    if (R_SUCCEEDED(rc)) rc = usbAddEndpoint(interface_index, EP_IN, &endpoint_descriptor_in);
    if (R_SUCCEEDED(rc)) rc = usbAddEndpoint(interface_index, EP_OUT, &endpoint_descriptor_out);
    if (R_SUCCEEDED(rc)) rc = usbEnableInterface(interface_index);
    return rc;
}

ssize_t USBCDCDataInterface::read(char *ptr, size_t len)
{
    return usbTransfer(interface_index, EP_OUT, UsbDirection_Read, (void*)ptr, len, 1000000000LL/*U64_MAX*/);
}
ssize_t USBCDCDataInterface::write(const char *ptr, size_t len)
{
    return usbTransfer(interface_index, EP_IN, UsbDirection_Write, (void*)ptr, len, 1000000000LL/*U64_MAX*/);
}

void USBCDCDataInterface::handle_input_packet(int data_interface_index, int com_interface_index, Log::Logger *logger, bool *quit)
{
    char *buffer = (char*)memalign(0x1000, 16384);
    while(!*quit)
    {
        USBCDCComInterface* com_interface = dynamic_cast<USBCDCComInterface*>(getInterface(com_interface_index)); // Do something with com_interface
        USBCDCDataInterface* data_interface = dynamic_cast<USBCDCDataInterface*>(getInterface(data_interface_index));
        if(com_interface != NULL && data_interface != NULL)
        {
            int size = data_interface->read(buffer, 16384);
            if(size > 0)
            {
                int len = data_interface->write(buffer, size);
                if(len != size)
                    logger->error() << "Loopback failed";
                logger->info()  << "Got data on interface " << data_interface->getInterfaceIndex();
                dump(buffer, size, logger);
            }
        }
    }
    free(buffer);
}