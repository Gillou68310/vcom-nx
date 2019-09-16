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

#ifndef __USB_H
#define __USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

#define TOTAL_INTERFACES 4
#define TOTAL_ENDPOINTS 4

typedef enum {
    UsbDirection_Read  = 0,
    UsbDirection_Write = 1,
} UsbDirection;

struct usb_interface_association_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bFirstInterface;
    uint8_t  bInterfaceCount;
    uint8_t  bFunctionClass;
    uint8_t  bFunctionSubClass;
    uint8_t  bFunctionProtocol;
    uint8_t  iFunction;
};

Result usbInitialize(struct usb_device_descriptor *device_descriptor, const char *manufacturer, const char *product, const char *serialNumber);
Result usbEnable(void);
void usbExit(void);

Result usbAddStringDescriptor(u8* out_index, const char* string);
Result usbInterfaceInit(u32 intf_ind, struct usb_interface_descriptor *interface_descriptor, struct usb_interface_association_descriptor *interface_association_descriptor);
Result usbInterfaceAddData(u32 intf_ind, char* data);
Result usbAddEndpoint(u32 intf_ind, u32 ep_ind, struct usb_endpoint_descriptor *endpoint_descriptor);
Result usbEnableInterface(u32 intf_ind);

Result usbGetSetupPacket(u32 intf_ind, void* buffer, size_t size, u64 timeout);
size_t usbTransfer(u32 intf_ind, u32 ep_ind, UsbDirection dir, void* buffer, size_t size, u64 timeout);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __USB_H */
