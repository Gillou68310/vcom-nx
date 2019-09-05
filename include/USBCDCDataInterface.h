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

#ifndef __USB_CDC_DATA_INTERFACE_H
#define __USB_CDC_DATA_INTERFACE_H

#include <LoggerCpp/LoggerCpp.h>
#include "USBInterface.h"

class USBCDCDataInterface : public USBInterface {
private:
    struct usb_interface_descriptor interface_descriptor = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bNumEndpoints = 2,
        .bInterfaceClass = 0x0A,
        .bInterfaceSubClass = 0x00,
        .bInterfaceProtocol = 0x00,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_in = {
       .bLength = USB_DT_ENDPOINT_SIZE,
       .bDescriptorType = USB_DT_ENDPOINT,
       .bEndpointAddress = USB_ENDPOINT_IN,
       .bmAttributes = USB_TRANSFER_TYPE_BULK,
       .wMaxPacketSize = 0x200,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_out = {
       .bLength = USB_DT_ENDPOINT_SIZE,
       .bDescriptorType = USB_DT_ENDPOINT,
       .bEndpointAddress = USB_ENDPOINT_OUT,
       .bmAttributes = USB_TRANSFER_TYPE_BULK,
       .wMaxPacketSize = 0x200,
    };
    
public:
            USBCDCDataInterface(int index);
    virtual ~USBCDCDataInterface();

    Result initialize();
    ssize_t read(char *ptr, size_t len);
    ssize_t write(const char *ptr, size_t len);
    static void handle_input_packet(int data_interface_index, int com_interface_index, Log::Logger *logger, bool *quit);
};

#endif /* __USB_CDC_DATA_INTERFACE_H */
