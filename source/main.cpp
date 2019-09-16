#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <LoggerCpp/LoggerCpp.h>

#include "USBCDCComInterface.h"
#include "USBCDCDataInterface.h"

static std::mutex mtx;
static bool quit = false;
USBInterface* USBInterface::interfaces[TOTAL_INTERFACES] = {NULL};

int main(int argc, char* argv[])
{
    int c;
    int verbose_level = 0;
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
              verbose_level = atoi(optarg);
              break;
            default:
              break;
        }
    }

    consoleInit(NULL);

    Log::Log::Level level = Log::Log::eNotice;
    if (verbose_level == 1) level = Log::Log::eInfo;
    else if (verbose_level > 1) level = Log::Log::eDebug;
    Log::Manager::setDefaultLevel(level);
    Log::Config::Vector configList;
    Log::Config::addOutput(configList, "OutputConsole");
    Log::Logger logger("VCOM", &mtx);

    try {
        Log::Manager::configure(configList);
    }
    catch (std::exception& e) {
        std::cout << e.what();
    }

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

    USBCDCDataInterface cdc_data_interface(1);
    USBCDCComInterface cdc_com_interface(0, &cdc_data_interface);
    USBCDCDataInterface cdc_data_interface1(3);
    USBCDCComInterface cdc_com_interface1(2, &cdc_data_interface1);

    std::thread setup(USBCDCComInterface::handle_setup_packet,
                        &logger, &quit);

    std::thread data(USBCDCDataInterface::handle_input_packet,
                        cdc_data_interface.getInterfaceIndex(),
                        cdc_com_interface.getInterfaceIndex(),
                        &logger, &quit);

    std::thread data1(USBCDCDataInterface::handle_input_packet,
                        cdc_data_interface1.getInterfaceIndex(),
                        cdc_com_interface1.getInterfaceIndex(),
                        &logger, &quit);

    Result rc = usbInitialize(&device_descriptor, "Nintendo",
                                "Nintendo Switch", "SerialNumber");
                                
    if(R_SUCCEEDED(rc)) rc = cdc_com_interface.initialize();
    if(R_SUCCEEDED(rc)) rc = cdc_data_interface.initialize();
    if(R_SUCCEEDED(rc)) rc = cdc_com_interface1.initialize();
    if(R_SUCCEEDED(rc)) rc = cdc_data_interface1.initialize();
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
            
            mtx.lock();
            std::cout.flush();
            consoleUpdate(NULL);
            mtx.unlock();
        }
    }

    Log::Manager::terminate();
    consoleExit(NULL);
    usbExit();
    return 0;
}

#define DUMP_BYTES_PER_ROW  16
void dump(void* buffer, int size, Log::Logger *logger)
{
    char* tmp_buffer = (char*)malloc(DUMP_BYTES_PER_ROW*4);
    char* bufptr = tmp_buffer;

    for (int i = 0; i < size; i++) {
        sprintf(bufptr, "%02X ", ((char*)buffer)[i]);
        bufptr += strlen(bufptr);
        if (i % DUMP_BYTES_PER_ROW == (DUMP_BYTES_PER_ROW - 1)) {
            logger->debug() << tmp_buffer;
            bufptr = tmp_buffer;
        }
    }
    if (bufptr != tmp_buffer) {
        // print last line
        logger->debug() << tmp_buffer;
    }
    free(tmp_buffer);
}
