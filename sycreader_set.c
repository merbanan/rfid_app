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

protocol description:

example write command:
0000   01 00 00 00 00 00 0e 00 aa 00 09 82 07 00 00 00
0010   00 00 00 00 8c bb 00 00 00 00 00 00 00 00 00 00

example read command:
0000   01 00 00 00 00 00 06 00 aa 00 01 83 82 bb 00 00


Read settings payload description:

00-07   03 00 00 00 00 00 00 00 <- unknown always the same
08      aa                      <- STX payload start marker, always the same
09      00                      <- station id, always the same
0A      0a                      <- size, always 0xa in this packet
0B-0C   00 00                   <- unknown always the same
0D      0X                      <- f = 10 digits in decimal (last four bytes)
                                <- c = 8 digits in hexadecimal
                                <- b = 8 digits in decimal (last 3 bytes)
                                <- 9 = 8 digits in decimal (last 4 bytes)
                                <- 6 = 18 digits in decimal (last 4 bytes)
                                <- 7 = 10 digits in hexadecimal
                                <- a = 10 digits in decimal in reverse
                                <- 3 = 8 digits in hexadecimal in reverse
                                <- d = 00 + 8 digits in decimal (last 3 bytes)
                                <- 8 = 5 digits in decimal
                                <- 5 = 13 digits in decimal
                                <- 0 = 2H4D + 2H4D
                                <- e = 8 digits in decimal with split (last 3 bytes)
0E      00                      <- 01 to prepend semicolon (;)
0F      00                      <- 01 to postpend questionmark (?)
10      00                      <- 01 to add comma split in the middle(,) (valid for last 2 output options)
11      00                      <- 01 to add enter after output
12-14   00                      <- unknown always the same
15      XX                      <- linear xor over the payload buffer (0B-14)
16      bb                      <- ETX payload stop marker, always the same

Write settings payload description:


00-07   01 00 00 00 00 00 0e 00 <- unknown always the same
08      aa                      <- STX payload start marker, always the same
09      00                      <- station id, always the same
0A      09                      <- size, always 0xa in this packet
0B      82                      <- unknown always the same
0C      0X                      <- f = 10 digits in decimal (last four bytes)
                                <- c = 8 digits in hexadecimal
                                <- b = 8 digits in decimal (last 3 bytes)
                                <- 9 = 8 digits in decimal (last 4 bytes)
                                <- 6 = 18 digits in decimal (last 4 bytes)
                                <- 7 = 10 digits in hexadecimal
                                <- a = 10 digits in decimal in reverse
                                <- 3 = 8 digits in hexadecimal in reverse
                                <- d = 00 + 8 digits in decimal (last 3 bytes)
                                <- 8 = 5 digits in decimal
                                <- 5 = 13 digits in decimal
                                <- 0 = 2H4D + 2H4D
                                <- e = 8 digits in decimal with split (last 3 bytes)
0D      00                      <- 01 to prepend semicolon (;)
0E      00                      <- 01 to postpend questionmark (?)
0F      00                      <- 01 to add comma split in the middle(,) (valid for last 2 output options)
10      00                      <- 01 to add enter after output
11-13   00                      <- unknown always the same
14      XX                      <- linear xor over the payload buffer (0B-14)
15      bb                      <- ETX payload stop marker, always the same




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
#define CMD_WRITE_SETTINGS      0x82
#define CMD_READ_SETTINGS       0x83
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
    fprintf(stdout,"-m [mode]\n");

    fprintf(stdout,"\t0  2H4D + 2H4D\n");
    fprintf(stdout,"\t3  8 digits in hexadecimal in reverse\n");
    fprintf(stdout,"\t5  13 digits in decimal\n");
    fprintf(stdout,"\t6  18 digits in decimal (last 4 bytes)\n");
    fprintf(stdout,"\t7  10 digits in hexadecimal\n");
    fprintf(stdout,"\t8  5 digits in decimal\n");
    fprintf(stdout,"\t9  8 digits in decimal (last 4 bytes)\n");
    fprintf(stdout,"\t10 10 digits in decimal in reverse\n");
    fprintf(stdout,"\t11 8 digits in decimal (last 3 bytes)\n");
    fprintf(stdout,"\t12 8 digits in hexadecimal\n");
    fprintf(stdout,"\t13 00 + 8 digits in decimal (last 3 bytes)\n");
    fprintf(stdout,"\t14 8 digits in decimal with split (last 3 bytes)\n");
    fprintf(stdout,"\t15 10 digits in decimal (last four bytes)\n\n");
    fprintf(stdout, "-s Prepend semicolon (;)\n");
    fprintf(stdout, "-q Postpend questionmark (?)\n");
    fprintf(stdout, "-l Comma split in the middle (,)\n");
    fprintf(stdout, "-e Add enter after output\n\n");
    fprintf(stdout, "Example: sycreader_set -w -m 7 -e\n");
    fprintf(stdout, "         sycreader_set -r\n");

}

static int calc_crc(uint8_t* buf, unsigned int len)
{
    int i;
    uint8_t crc = 0x0;

    for (i=0 ; i<len ; i++) {
        crc ^= buf[i]; 
    }
    return crc;
}

static int create_out_packet(uint8_t* pkt, int cmd, uint8_t* data, int data_size)
{
    int i;

    /* Populate type header for out packet, default unknown data */
    pkt[0]  = 0x01;
    pkt[1]  = 0x00;
    pkt[2]  = 0x00;
    pkt[3]  = 0x00;
    pkt[4]  = 0x00;
    pkt[5]  = 0x00;
    pkt[6]  = 0x00;
    pkt[7]  = 0x00;

    /* Populate command payload */
    pkt[8]  = STX_AA;
    pkt[9]  = STATION_ID;
    switch (cmd) {
    case CMD_READ_SETTINGS:
        if (verbose) fprintf(stdout, "CMD_READ_SETTINGS\n");
        pkt[6]  = 0x06;
        pkt[10] = 1;
        pkt[11] = CMD_READ_SETTINGS;
        pkt[12] = calc_crc(&pkt[9], 1+1+pkt[10]);
        pkt[13] = ETX_BB;
        break;
    case CMD_WRITE_SETTINGS:
        if (verbose) fprintf(stdout, "CMD_WRITE\n");
        pkt[6]  = 0x0e;
        pkt[10] = data_size+1;
        pkt[11] = CMD_WRITE_SETTINGS;
        memcpy(&pkt[12], data, data_size);
        pkt[10+data_size+2] = calc_crc(&pkt[9], 1+1+pkt[10]);
        pkt[10+data_size+3] = ETX_BB;
        break;
    default:
        fprintf(stderr, "Unknown command: 0x%02x\n", cmd);
        return -1;
    }
};

static int prepare_settings(uint8_t *db, int mode,
           int semicolon, int questionmark, int split, int enter) {
    db[0]  = mode;
    db[1]  = semicolon;
    db[2]  = questionmark;
    db[3]  = split;
    db[4]  = enter;
    db[5]  = 0x0;
    db[6]  = 0x0;
    db[7]  = 0x0;


};

static int send_write_settings(struct libusb_device_handle *devh, int mode,
           int semicolon, int questionmark, int split, int enter) {
    uint8_t write_buffer[WRITE_BUF_SIZE] = {0};
    uint8_t out_pkt[PACKET_CTRL_LEN] = {0};
    uint8_t answer[PACKET_CTRL_LEN] = {0};
    uint8_t c[6] = {0};
    int r, i;
 
    /* Prepare write packet */
    prepare_settings(write_buffer, mode, semicolon, questionmark, split, enter);
    create_out_packet(out_pkt, CMD_WRITE_SETTINGS, write_buffer, 8);
//    handle_packet(out_pkt, NULL);

    /* Send read packet */
    r = libusb_control_transfer(devh,CTRL_OUT,HID_SET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x01,0,out_pkt, PACKET_CTRL_LEN,TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control Out error %d\n", r); 
        return r; 
    }
    /* Get answer */
    r = libusb_control_transfer(devh,CTRL_IN,HID_GET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x01,0, answer,PACKET_CTRL_LEN, TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control IN error %d\n", r); 
        return r; 
    }

    /* Print data */
    for (i=0 ; i<32 ; i++) {
        if(i%16 == 0) 
            if (verbose) fprintf(stdout,"\n"); 
        if (verbose) fprintf(stdout, "%02x ", out_pkt[i]); 
    }
    if (verbose) fprintf(stdout, "\n");


};

static int send_read_settings(struct libusb_device_handle *devh) {
    uint8_t out_pkt[PACKET_CTRL_LEN] = {0};
    uint8_t answer[PACKET_CTRL_LEN] = {0};
    uint8_t c[6] = {0};
    int r, i;
    char *mode;

    /* Prepare read packet */
    create_out_packet(out_pkt, CMD_READ_SETTINGS, NULL, 0);
//    handle_packet(out_pkt, NULL);

    /* Send read packet */
    r = libusb_control_transfer(devh,CTRL_OUT,HID_SET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x01,0,out_pkt, PACKET_CTRL_LEN,TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control Out error %d\n", r); 
        return r; 
    }
    /* Get answer */
    r = libusb_control_transfer(devh,CTRL_IN,HID_GET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x01,0, answer,PACKET_CTRL_LEN, TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control IN error %d\n", r); 
        return r; 
    }

    /* Print data */
    for (i=0 ; i<32 ; i++) {
        if(i%16 == 0) 
            if (verbose) fprintf(stdout,"\n"); 
        if (verbose) fprintf(stdout, "%02x ", answer[i]); 
    }
    if (verbose) fprintf(stdout, "\n");

    /* parse settings */
    switch (answer[0x0d]) {
        case 0x0: mode = "2H4D + 2H4D"; break;
        case 0x3: mode = "8 digits in hexadecimal in reverse"; break;
        case 0x5: mode = "13 digits in decimal"; break;
        case 0x6: mode = "18 digits in decimal (last 4 bytes)"; break;
        case 0x7: mode = "10 digits in hexadecimal"; break;
        case 0x8: mode = "5 digits in decimal"; break;
        case 0x9: mode = "8 digits in decimal (last 4 bytes)"; break;
        case 0xa: mode = "10 digits in decimal in reverse"; break;
        case 0xb: mode = "8 digits in decimal (last 3 bytes)"; break;
        case 0xc: mode = "8 digits in hexadecimal"; break;
        case 0xd: mode = "00 + 8 digits in decimal (last 3 bytes)"; break;
        case 0xe: mode = "8 digits in decimal with split (last 3 bytes)"; break;
        case 0xf: mode = "10 digits in decimal (last four bytes)"; break;
        default: mode = "unknown mode"; break;
    }
    fprintf(stdout, "Mode: %s\n", mode);

    fprintf(stdout, "Prepend semicolon (;):\t\t%d\n", answer[0x0e]);
    fprintf(stdout, "Postpend questionmark (?):\t%d\n", answer[0x0f]);
    fprintf(stdout, "Comma split in the middle (,):\t%d\n", answer[0x10]);
    fprintf(stdout, "Add enter after output:\t\t%d\n", answer[0x11]);

    return 0;
}

int main(int argc,char** argv)
{ 
    int r = 1, i;
    struct libusb_device_handle *devh = NULL; 

    int option = 0;
    int read_device = 0;
    int write_device = 0;
    int mode=0, semicolon=0, questionmark=0, split=0, enter=0;
    char* write_string = NULL;

    while ((option = getopt(argc, argv,"wvrm:sqle")) != -1) {
        switch (option) {
            case 'v' : verbose = 1;
                break;
            case 'r' : read_device = 1;
                break;
            case 'm' : mode = atoi(optarg);
                break;
            case 's' : semicolon = 1;
                break;
            case 'q' : questionmark = 1;
                break;
            case 'l' : split = 1;
                break;
            case 'e' : enter = 1;
                break;
            case 'w' : write_device = 1;
                break;
            default: print_usage(); 
                 exit(EXIT_FAILURE);
        }
    }
    if (argc < 2) {
        print_usage();
        goto exit;
    }

    if (verbose) fprintf(stdout, "Init usb\n"); 

    /* Init USB */
    r = libusb_init(NULL); 
    if (r < 0) { 
        fprintf(stderr, "Failed to initialise libusb\n"); 
        exit(1); 
    } 

    devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (!devh) {
        if (verbose) fprintf(stdout, "USB device open failed\n"); 
        goto out;
    } 
    if (verbose) fprintf(stdout, "Successfully found the RFID R/W device\n"); 

    r = libusb_detach_kernel_driver(devh, 0); 
    if (r < 0) { 
//        fprintf(stderr, "libusb_detach_kernel_driver error %d\n", r); 
//        goto out; 
    }


    r = libusb_claim_interface(devh, 0); 
    if (r < 0) { 
        fprintf(stderr, "libusb_claim_interface error %d\n", r); 
        goto out; 
    }

    if (read_device) {
        send_read_settings(devh);
    }

    if (write_device) {
        send_write_settings(devh, mode, semicolon, questionmark, split, enter);
    }

    libusb_release_interface(devh, 0); 
out: 
    //	libusb_reset_device(devh); 
    libusb_close(devh); 
    libusb_exit(NULL); 
exit:
    return r >= 0 ? r : -r; 
}
