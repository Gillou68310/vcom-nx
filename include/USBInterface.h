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

#ifndef __USB_INTERFACE_H
#define __USB_INTERFACE_H

#include <assert.h>
#include "usb.h"

class USBInterface {
protected:
    static USBInterface *interfaces[TOTAL_INTERFACES];
    int interface_index;

public:
    USBInterface(int index) {
        assert(index < TOTAL_INTERFACES);
        interface_index = index;
        interfaces[index] = this;
    }
    virtual Result initialize() = 0;
    int getInterfaceIndex(void) {return interface_index;}
    static USBInterface * getInterface(int index)
    {
        if(index < TOTAL_INTERFACES)
            return interfaces[index];
        else
            return NULL;
    }
};

#endif /* __USB_INTERFACE_H */
