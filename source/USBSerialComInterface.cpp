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

#include "USBSerialComInterface.h"

void dump(void* buffer, int size);

 struct usb_ctrlrequest {
   uint8_t bRequestType;
   uint8_t bRequest;
   uint16_t wValue;
   uint16_t wIndex;
   uint16_t wLength;
 } PACKED;

static const char parity_char[] = {'N', 'O', 'E', 'M', 'S'};
static const char *stop_bits_str[] = {"1","1.5","2"};

#define EP0 0xFFFFFFFF
#define EP_INT 0

#define GET_LINE_CODING           0x21
#define SET_LINE_CODING           0x20
#define SET_CONTROL_LINE_STATE    0x22
#define SEND_BREAK                0x23

USBSerialComInterface::USBSerialComInterface(int index, USBSerialDataInterface *cdc_data_interface) : USBInterface(index)
{
    interface_association_descriptor.bFirstInterface = getInterfaceIndex();
    cdc_com_call_mgmt.bDataInterface = cdc_data_interface->getInterfaceIndex();
    cdc_com_union.bControlInterface = getInterfaceIndex();
    cdc_com_union.bSubordinateInterface0 = cdc_data_interface->getInterfaceIndex();
}

USBSerialComInterface::~USBSerialComInterface() {
}

Result USBSerialComInterface::initialize(const char* str)
{
    Result rc = usbAddStringDescriptor(&interface_association_descriptor.iFunction, str);
    if (R_SUCCEEDED(rc)) rc = usbAddStringDescriptor(&interface_descriptor.iInterface, "CDC Abstract Control Model (ACM)");
    if (R_SUCCEEDED(rc)) rc = usbInterfaceInit(getInterfaceIndex(), &interface_descriptor, &interface_association_descriptor);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(getInterfaceIndex(), (char*)&cdc_com_header);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(getInterfaceIndex(), (char*)&cdc_com_call_mgmt);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(getInterfaceIndex(), (char*)&cdc_com_acm);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(getInterfaceIndex(), (char*)&cdc_com_union);
    if (R_SUCCEEDED(rc)) rc = usbAddEndpoint(getInterfaceIndex(), EP_INT, &endpoint_descriptor_interrupt);
    if (R_SUCCEEDED(rc)) rc = usbEnableInterface(getInterfaceIndex());
    return rc;
}

ssize_t USBSerialComInterface::sendEvent(const char *ptr, size_t len, u64 timestamp)
{
    return usbTransfer(getInterfaceIndex(), EP_INT, UsbDirection_Write, (void*)ptr, len, timestamp);
}

void USBSerialComInterface::handle_setup_packet(bool *quit)
{
    int size;
    Result rc;
    struct usb_ctrlrequest ctrl;
    
    while(!*quit)
    {
        rc = usbGetSetupPacket(0, &ctrl, sizeof(struct usb_ctrlrequest), 1000000000LL);
        if (R_SUCCEEDED(rc)) {
            VLOG(1) << "Got setup packet on interface " << ctrl.wIndex;
            dump(&ctrl, sizeof(struct usb_ctrlrequest));

            USBSerialComInterface* com_interface = dynamic_cast<USBSerialComInterface*>(getInterface(ctrl.wIndex));
            if(com_interface == NULL)
            {
                LOG(ERROR) << "Not a comm interface " << ctrl.wIndex;
                continue;
            }

            int interface = com_interface->getInterfaceIndex();
            struct line_coding *coding = com_interface->getLineCoding();

            switch (ctrl.bRequest) {
                case GET_LINE_CODING:
                    VLOG(1) << "Get line coding";
                    size = usbTransfer(interface, EP0, UsbDirection_Write, coding, sizeof(struct line_coding), U64_MAX);
                    if(size == sizeof(struct line_coding))
                    {
                        size = usbTransfer(interface, EP0, UsbDirection_Read, NULL, 0, U64_MAX);
                        VLOG(2) << "Read ZLP: " << size;
                    }
                    else
                    {
                        // TODO: usbDsInterface_StallCtrl?
                        LOG(ERROR) << "Get line coding failed";
                    }
                    break;
                case SET_LINE_CODING:
                    VLOG(1) << "Set line coding";
                    size = usbTransfer(interface, EP0, UsbDirection_Read, coding, sizeof(struct line_coding), U64_MAX);
                    dump(coding, sizeof(struct line_coding));
                    if (size == sizeof(struct line_coding))
                    {
                        char buf[256];
                        char parity = (coding->bParityType > 4) ? '?' : parity_char[coding->bParityType];
                        const char *stop_bits = (coding->bCharFormat > 2) ? "?" : stop_bits_str[coding->bCharFormat];
                        sprintf(buf, "%d/%d/%c/%s",coding->dwDTERate, coding->bDataBits, parity, stop_bits);
                        VLOG(1) << buf;
                        size = usbTransfer(interface, EP0, UsbDirection_Write, NULL, 0, U64_MAX);
                        VLOG(2) << "Send ZLP: " << size;
                    }
                    else
                    {
                        // TODO: usbDsInterface_StallCtrl?
                       LOG(ERROR) << "Set control line state failed";
                    }
                    break;
                case SET_CONTROL_LINE_STATE:
                    VLOG(1) << "Set control line state";
                    if(ctrl.wValue & 0x02) {VLOG(1) << "Carrier on"; com_interface->setCarrier(true);}
                    else {VLOG(1) << "Carrier off"; com_interface->setCarrier(false);}
                    if(ctrl.wValue & 0x01) {VLOG(1) << "DTE on"; com_interface->setDTE(true);}
                    else {VLOG(1) << "DTE off"; com_interface->setDTE(false);}
                    size = usbTransfer(interface, EP0, UsbDirection_Write, NULL, 0, U64_MAX);
                    VLOG(2) << "Send ZLP: " << size;
                    break;    
                default:
                    LOG(ERROR) << "Unknown setup command";
                    break;
            }
        }
    }
}