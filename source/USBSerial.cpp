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

#include "USBSerial.h"

USBInterface* USBInterface::interfaces[TOTAL_INTERFACES] = {NULL};
int USBSerial::interface_count = 0;

USBSerial::USBSerial() {
	index = interface_count >> 1;
	data = new USBSerialDataInterface(interface_count + 1);
	com = new USBSerialComInterface(interface_count, data);
    interface_count += 2;
}

Result USBSerial::initialize(const char* str)
{
	Result rc = com->initialize(str);
	if(R_SUCCEEDED(rc)) rc = data->initialize(nullptr);
	return rc;
}

ssize_t USBSerial::read(char *ptr, size_t len)
{
	if(com->getDTE())
		return data->read(ptr, len, 1000000000LL/*U64_MAX*/);
	else
		return -1;
}

ssize_t USBSerial::write(const char *ptr, size_t len)
{
	if(com->getDTE())
		return data->write(ptr, len, 1000000000LL/*U64_MAX*/);
	else
		return -1;
}

ssize_t USBSerial::sendEvent(const char *ptr, size_t len)
{
	if(com->getDTE())
		return com->sendEvent(ptr, len, 1000000000LL/*U64_MAX*/);
	else
		return -1;
}

USBSerial::~USBSerial() {
	delete com;
	delete data;
}