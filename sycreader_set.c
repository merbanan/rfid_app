/*
Copyright (C) 2017 Benjamin Larsson <benjamin.larsson@inteno.se>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
Bus 002 Device 048: ID ffff:0035  
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               1.10
  bDeviceClass            0 (Defined at Interface level)
  bDeviceSubClass         0 
  bDeviceProtocol         0 
  bMaxPacketSize0         8
  idVendor           0xffff 
  idProduct          0x0035 
  bcdDevice            1.00
  iManufacturer           1 Sycreader RFID Technology Co., Ltd
  iProduct                2 SYC ID&IC USB Reader
  iSerial                 3 08FF20140315
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength           52
    bNumInterfaces          2
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0xa0
      (Bus Powered)
      Remote Wakeup
    MaxPower              100mA
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           1
      bInterfaceClass         3 Human Interface Device
      bInterfaceSubClass      1 Boot Interface Subclass
      bInterfaceProtocol      1 Keyboard
      iInterface              4 USB Standard Keyboard
        HID Device Descriptor:
          bLength                 9
          bDescriptorType        33
          bcdHID               1.10
          bCountryCode           33 US
          bNumDescriptors         1
          bDescriptorType        34 Report
          wDescriptorLength      65
         Report Descriptors: 
           ** UNAVAILABLE **
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0008  1x 8 bytes
        bInterval              10
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       0
      bNumEndpoints           0
      bInterfaceClass         3 Human Interface Device
      bInterfaceSubClass      0 No Subclass
      bInterfaceProtocol      0 None
      iInterface              5 USB Vender Hid
        HID Device Descriptor:
          bLength                 9
          bDescriptorType        33
          bcdHID               1.10
          bCountryCode           33 US
          bNumDescriptors         1
          bDescriptorType        34 Report
          wDescriptorLength      35
          Report Descriptor: (length is 35)
            Item(Global): Usage Page, data= [ 0xa0 0xff ] 65440
                            (null)
            Item(Local ): Usage, data= [ 0x01 ] 1
                            (null)
            Item(Main  ): Collection, data= [ 0x01 ] 1
                            Application
            Item(Global): Report ID, data= [ 0x01 ] 1
            Item(Local ): Usage, data= [ 0x08 ] 8
                            (null)
            Item(Global): Logical Maximum, data= [ 0xff 0x00 ] 255
            Item(Global): Logical Minimum, data= [ 0x00 ] 0
            Item(Global): Report Size, data= [ 0x08 ] 8
            Item(Global): Report Count, data= [ 0xff ] 255
            Item(Main  ): Feature, data= [ 0x02 ] 2
                            Data Variable Absolute No_Wrap Linear
                            Preferred_State No_Null_Position Non_Volatile Bitfield
            Item(Global): Report ID, data= [ 0x02 ] 2
            Item(Local ): Usage, data= [ 0x08 ] 8
                            (null)
            Item(Main  ): Feature, data= [ 0x02 ] 2
                            Data Variable Absolute No_Wrap Linear
                            Preferred_State No_Null_Position Non_Volatile Bitfield
            Item(Global): Report ID, data= [ 0x03 ] 3
            Item(Local ): Usage, data= [ 0x08 ] 8
                            (null)
            Item(Main  ): Feature, data= [ 0x02 ] 2
                            Data Variable Absolute No_Wrap Linear
                            Preferred_State No_Null_Position Non_Volatile Bitfield
            Item(Main  ): End Collection, data=none
Device Status:     0x0000
  (Bus Powered)

  */

#include <errno.h>
#include <signal.h> 
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <getopt.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h> 

#define VENDOR_ID 0xffff
#define PRODUCT_ID 0x0035

// HID Class-Specific Requests values. See section 7.2 of the HID specifications 
#define HID_GET_REPORT                0x01 
#define HID_GET_IDLE                  0x02 
#define HID_GET_PROTOCOL              0x03 
#define HID_SET_REPORT                0x09 
#define HID_SET_IDLE                  0x0A 
#define HID_SET_PROTOCOL              0x0B 
#define HID_REPORT_TYPE_INPUT         0x01 
#define HID_REPORT_TYPE_OUTPUT        0x02 
#define HID_REPORT_TYPE_FEATURE       0x03 

#define CTRL_IN      LIBUSB_ENDPOINT_IN |LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE 
#define CTRL_OUT     LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE 

#define PACKET_CTRL_LEN 0x100
#define WRITE_BUF_SIZE 25

#define OUT_PKT     0x01
#define IN_PKT      0x03

#define CMD_READ    0x20
#define CMD_WRITE   0x21
#define CMD_GET_SNR 0x25
#define CMD_LED1    0x87
#define CMD_LED2    0x88
#define CMD_BUZZER  0x89

#define STATUS_OK   0x00
#define STATUS_FAIL 0x01

#define STX_AA      0xaa
#define ETX_BB      0xbb
#define STATION_ID  0x00

const static int PACKET_INT_LEN=2;
const static int INTERFACE=0; 
const static int ENDPOINT_INT_IN=0x00; /* endpoint 0x81 address for IN */ 
const static int ENDPOINT_INT_OUT=0x00; /* endpoint 0 address for OUT */ 
const static int TIMEOUT=5000; /* timeout in ms */ 

static int verbose = 0;

static void print_usage() {
    fprintf(stdout,"Sycreader_set version 1.0, Copyright (c) 2017 Benjamin Larsson <benjamin.larsson@inteno.se>\n");
    fprintf(stdout,"Usage: sycreader_set [OPTION]...\n");
\"\n");
}

static int send_config_set(struct libusb_device_handle *devh, int reader_setting) {
    
    
}


int main(int argc,char** argv)
{ 
    int r = 1, i;


    uint8_t rfid_buf[5] = { 0x0f, 0x00, 0x22, 0x3c, 0x26 };
    struct libusb_device_handle *devh = NULL; 

    int option = 0;
    int read_device = 0;
    int format = 0;
    char* write_string = NULL;

    while ((option = getopt(argc, argv,"vrf:w:")) != -1) {
        switch (option) {
            case 'v' : verbose = 1;
                break;
            case 'r' : read_device = 1;
                break;
            case 'f' : format = atoi(optarg);
                break;
            case 'w' : write_string = optarg;
                break;
            default: print_usage(); 
                 exit(EXIT_FAILURE);
        }
    }
    if (argc < 2) {
        print_usage();
        goto exit;
    }


    /* Init USB */
    r = libusb_init(NULL); 
    if (r < 0) { 
        fprintf(stderr, "Failed to initialise libusb\n"); 
        exit(1); 
    } 

    devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (!devh)
        goto out; 
    if (verbose) fprintf(stdout, "Successfully found the RFID R/W device\n"); 

    r = libusb_set_configuration(devh, 1); 
    if (r < 0) { 
        fprintf(stderr, "libusb_set_configuration error %d\n", r); 
        goto out; 
    }
    libusb_release_interface(devh, 0); 
out: 
    //	libusb_reset_device(devh); 
    libusb_close(devh); 
    libusb_exit(NULL); 
exit:
    return r >= 0 ? r : -r; 
}
