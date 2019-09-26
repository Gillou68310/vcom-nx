#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <malloc.h>
#include <thread>
#include <mutex>
#include <iostream>
#include "Logger.h"

#include "USBSerial.h"

static bool quit = false;

#define DUMP_BYTES_PER_ROW  16
void dump(void* buffer, int size)
{
    char* tmp_buffer = (char*)malloc(DUMP_BYTES_PER_ROW*4);
    char* bufptr = tmp_buffer;

    for (int i = 0; i < size; i++) {
        sprintf(bufptr, "%02X ", ((char*)buffer)[i]);
        bufptr += strlen(bufptr);
        if (i % DUMP_BYTES_PER_ROW == (DUMP_BYTES_PER_ROW - 1)) {
            VLOG(2) << tmp_buffer;
            bufptr = tmp_buffer;
        }
    }
    if (bufptr != tmp_buffer) {
        // print last line
        VLOG(2) << tmp_buffer;
    }
    free(tmp_buffer);
}

void handle_input_packet(USBSerial *serial, bool *quit)
{
    char *buffer = (char*)memalign(0x1000, 16384);
    while(!*quit)
    {
        int size = serial->read(buffer, 16384);
        if(size > 0)
        {
            int len = serial->write(buffer, size);
            if(len != size)
                LOG(ERROR) << "Loopback failed";
            LOG(INFO) << "Got data on serial " << serial->getIndex();
            dump(buffer, size);
        }
    }
    free(buffer);
}

int main(int argc, char* argv[])
{
    int c;
    struct option long_options[] =
    {
      {"verbose", required_argument, 0, 'v'},
      {0, 0, 0, 0}
    };

    while(1)
    {
        int option_index = 0;
        c = getopt_long (argc, argv, "v:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 'v':
              Log::verbose_level = atoi(optarg);
              break;
            default:
              break;
        }
    }

    consoleInit(NULL);

    std::cout << "Press + to exit\n";
    consoleUpdate(NULL);

    struct usb_device_descriptor device_descriptor = {
        .bLength = USB_DT_DEVICE_SIZE,
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0110,
        .bDeviceClass = 0xef,
        .bDeviceSubClass = 0x02,
        .bDeviceProtocol = 0x01,
        .bMaxPacketSize0 = 0x40,
        .idVendor = 0x057e,
        .idProduct = 0x5000,
        .bcdDevice = 0x0100,
        .bNumConfigurations = 0x01
    };

    USBSerial serial1;
    USBSerial serial2;

    std::thread setup(USBSerialComInterface::handle_setup_packet,
                     &quit);

    std::thread data(handle_input_packet,
                        &serial1,
                        &quit);

    std::thread data1(handle_input_packet,
                        &serial2,
                        &quit);

    Result rc = usbInitialize(&device_descriptor, "Nintendo",
                                "Nintendo Switch", "SerialNumber");
                                
    if(R_SUCCEEDED(rc)) rc = serial1.initialize("CDC Serial 1");
    if(R_SUCCEEDED(rc)) rc = serial2.initialize("CDC Serial 2");
    if(R_SUCCEEDED(rc)) rc = usbEnable();

    if(R_SUCCEEDED(rc))
    {
        while (appletMainLoop() && !quit)
        {
            hidScanInput();
            u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

            if (kDown & KEY_PLUS)
            {
                quit = true;
                setup.join();
                data.join();
                data1.join();
            }
            
            // Mutex is required because Console is not thread safe
            Log::mtx.lock();
            std::cout.flush();
            consoleUpdate(NULL);
            Log::mtx.unlock();
        }
    }

    consoleExit(NULL);
    usbExit();
    return 0;
}
