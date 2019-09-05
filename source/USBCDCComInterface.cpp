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

#include "USBCDCComInterface.h"

void dump(void* buffer, int size, Log::Logger *logger);

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

USBCDCComInterface::USBCDCComInterface(int index, USBCDCDataInterface *cdc_data_interface) : USBInterface(index)
{
    cdc_com_call_mgmt.bDataInterface = cdc_data_interface->getInterfaceIndex();
    cdc_com_union.bControlInterface = interface_index;
    cdc_com_union.bSubordinateInterface0 = cdc_data_interface->getInterfaceIndex();
}

USBCDCComInterface::~USBCDCComInterface() {
}

Result USBCDCComInterface::initialize()
{
    Result rc = usbAddStringDescriptor(&interface_descriptor.iInterface, "CDC Abstract Control Model (ACM)");
    if (R_SUCCEEDED(rc)) rc = usbInterfaceInit(interface_index, &interface_descriptor);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(interface_index, (char*)&cdc_com_header);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(interface_index, (char*)&cdc_com_call_mgmt);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(interface_index, (char*)&cdc_com_acm);
    if (R_SUCCEEDED(rc)) rc = usbInterfaceAddData(interface_index, (char*)&cdc_com_union);
    if (R_SUCCEEDED(rc)) rc = usbAddEndpoint(interface_index, EP_INT, &endpoint_descriptor_interrupt);
    if (R_SUCCEEDED(rc)) rc = usbEnableInterface(interface_index);
    return rc;
}

ssize_t USBCDCComInterface::sendEvent(const char *ptr, size_t len)
{
    return usbTransfer(interface_index, EP_INT, UsbDirection_Write, (void*)ptr, len, 1000000000LL/*U64_MAX*/);
}

void USBCDCComInterface::handle_setup_packet(Log::Logger *logger, bool *quit)
{
    int size;
    Result rc;
    struct usb_ctrlrequest ctrl;
    
    while(!*quit)
    {
        rc = usbGetSetupPacket(0, &ctrl, sizeof(struct usb_ctrlrequest), 1000000000LL);
        if (R_SUCCEEDED(rc)) {
            logger->info() << "Got setup packet on interface " << ctrl.wIndex;
            dump(&ctrl, sizeof(struct usb_ctrlrequest), logger);

            USBCDCComInterface* com_interface = dynamic_cast<USBCDCComInterface*>(getInterface(ctrl.wIndex));
            if(com_interface == NULL)
            {
                logger->error() << "Not a comm interface " << ctrl.wIndex;
                continue;
            }

            int interface = com_interface->getInterfaceIndex();
            struct line_coding *coding = com_interface->getLineCoding();

            switch (ctrl.bRequest) {
                case GET_LINE_CODING:
                    logger->info() << "Get line coding";
                    size = usbTransfer(interface, EP0, UsbDirection_Write, coding, sizeof(struct line_coding), U64_MAX);
                    if(size == sizeof(struct line_coding))
                    {
                        size = usbTransfer(interface, EP0, UsbDirection_Read, NULL, 0, U64_MAX);
                        logger->debug() << "Read ZLP: " << size;
                    }
                    else
                    {
                        // TODO: usbDsInterface_StallCtrl?
                        logger->error() << "Get line coding failed";
                    }
                    break;
                case SET_LINE_CODING:
                    logger->info() << "Set line coding";
                    size = usbTransfer(interface, EP0, UsbDirection_Read, coding, sizeof(struct line_coding), U64_MAX);
                    dump(coding, sizeof(struct line_coding), logger);
                    if (size == sizeof(struct line_coding))
                    {
                        char buf[256];
                        char parity = (coding->bParityType > 4) ? '?' : parity_char[coding->bParityType];
                        const char *stop_bits = (coding->bCharFormat > 2) ? "?" : stop_bits_str[coding->bCharFormat];
                        sprintf(buf, "%d/%d/%c/%s",coding->dwDTERate, coding->bDataBits, parity, stop_bits);
                        logger->info() << buf;
                    }
                    else
                    {
                        // TODO: usbDsInterface_StallCtrl?
                       logger->error() << "Set control line state failed";
                    }
                    break;
                case SET_CONTROL_LINE_STATE:
                    logger->info() << "Set control line state";
                    if(ctrl.wValue & 0x02) {logger->info() << "Carrier on"; com_interface->setCarrier(true);}
                    else {logger->info() << "Carrier off"; com_interface->setCarrier(false);}
                    if(ctrl.wValue & 0x01) {logger->info() << "DTE on"; com_interface->setDTE(true);}
                    else {logger->info() << "DTE off"; com_interface->setDTE(false);}
                    size = usbTransfer(interface, EP0, UsbDirection_Write, NULL, 0, U64_MAX);
                    logger->debug() << "Send ZLP: " << size;
                    break;    
                default:
                    logger->error() << "Unknown setup command";
                    break;
            }
        }
    }
}