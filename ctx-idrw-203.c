/*
Copyright (C) 2017 Benjamin Larsson <benjamin.larsson@inteno.se>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*

Bus 005 Device 016: ID 6688:6850  
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               2.00
  bDeviceClass            0 (Defined at Interface level)
  bDeviceSubClass         0 
  bDeviceProtocol         0 
  bMaxPacketSize0         8
  idVendor           0x6688 
  idProduct          0x6850 
  bcdDevice            1.01
  iManufacturer           1 CTX
  iProduct                2 203-ID-RW
  iSerial                 3 (error)
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength           41
    bNumInterfaces          1
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0xa0
      (Bus Powered)
      Remote Wakeup
    MaxPower              200mA
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           2
      bInterfaceClass         3 Human Interface Device
      bInterfaceSubClass      0 No Subclass
      bInterfaceProtocol      0 None
      iInterface              0 
        HID Device Descriptor:
          bLength                 9
          bDescriptorType        33
          bcdHID               1.10
          bCountryCode            0 Not supported
          bNumDescriptors         1
          bDescriptorType        34 Report
          wDescriptorLength      46
         Report Descriptors: 
           ** UNAVAILABLE **
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x85  EP 5 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0030  1x 48 bytes
        bInterval               1
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x03  EP 3 OUT
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0018  1x 24 bytes
        bInterval               1
Device Status:     0x0000
  (Bus Powered)

protocol description:

00	03		endpoint id (3 computer to usb device, 5 usb device to computer)
01	01		start marker
02	XX		total payload size [5 bytes header/trailer + command data size]
03	XX		payload command
04+	XX		command data
XX	XX		checksum value (xor from offset 01 to end of command data)
XX+1	04		end marker

Command			Data:

0x00	GetSupport		(0 bytes)
0x01 	TestDevice		(2 bytes): 16bit unknown value
0x03	Buzzer			(1 byte): 1-9, buzzer duration
0x10	Em4100read		(0 bytes):  returns 5 hex data from tag
0x12	T5577blockpasswrite	(11 bytes): unknown layout
0x12	T5577blockwrite	(7 bytes): unknown layout
0x12	T5577reset		(5 bytes): unknown layout
0x12	T5577readcurrent	(5 bytes): unknown layout
0x12	T5577pageread	(6 bytes): unknown layout
0x12	T5577blockread	(7 bytes): unknown layout
0x12	T5577wakeup		(6 bytes): unknown layout
0x13	Em4305login		(7 bytes): unknown layout
0x13	Em4305disable	(7 bytes): unknown layout
0x13	Em4305readword	(7 bytes): unknown layout
0x13	Em4305writeword	(7 bytes): unknown layout
0x13	Em4305protect	(7 bytes): unknown layout

0x20	mifare_reset		(0 bytes)


Usb payload is 24 bytes large, the unknown layout is most likely T5577/EM4305 standard based.


*/

#include <errno.h>
#include <signal.h> 
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <getopt.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h> 

#define VENDOR_ID 0x6688
#define PRODUCT_ID 0x6850

#define ENDPOINT_IN		0x85
#define ENDPOINT_OUT	0x03

#define MESSAGE_START_MARKER	0x01
#define MESSAGE_END_MARKER		0x04
#define MESSAGE_STRUCTURE_SIZE	5

/* Commands (from computer) */
#define CMD_BUZZER			0x03
#define CMD_EM4100ID_READ	0x10

/* Commands (to computer) */
#define CMD_EM4100ID_ANSWER	0x90
static int verbose = 0;

int timeout=1000; /* timeout in ms */

int prepare_message(uint8_t *out_buf, int endpoint, int command, uint8_t *pl_buf, int pl_buf_size) {

	int i,j;
	int x = 0;
	out_buf[0] = endpoint;
	out_buf[1] = MESSAGE_START_MARKER;
	out_buf[2] = 5 + pl_buf_size;	//size of message
	out_buf[3] = command;
	i = 4;
	if (pl_buf_size > 0) {
		memcpy(&out_buf[i], pl_buf, pl_buf_size);
		i+=pl_buf_size;
	}
	/* Calculate checksum byte */
	for (j=1 ; j<i ; j++) {
		x = x^out_buf[j];
	}
	out_buf[i] = x;
	out_buf[i+1] = MESSAGE_END_MARKER;


    for (i=0 ; i<24 ; i++) {
        if(i%16 == 0)
            if (verbose) fprintf(stdout,"\n");
        if (verbose) fprintf(stdout, "%02x ", out_buf[i]);
    }
    if (verbose) fprintf(stdout, "\n");

};

int send_message(struct libusb_device_handle * devh, uint8_t *message, uint8_t *answer) {
	int r,i;
	int bt = 0;
	r = libusb_interrupt_transfer(devh, ENDPOINT_OUT, message, 24, &bt, timeout);

	r = libusb_interrupt_transfer(devh, ENDPOINT_IN, answer, 48, &bt, timeout);

    if (verbose) fprintf(stdout, "Answer:\n");
    for (i=0 ; i<48 ; i++) {
        if(i%16 == 0)
            if (verbose) fprintf(stdout,"\n");
        if (verbose) fprintf(stdout, "%02x ", answer[i]);
    }
    if (verbose) fprintf(stdout, "\n");

};

int send_read_em4100id(struct libusb_device_handle * devh) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	int cmd_answer_size = 0;

	uint8_t buzz[1] = {9};
	prepare_message(cmd, ENDPOINT_OUT, CMD_BUZZER, &buzz[0], 1);
	memset(cmd, 0, 24);

	prepare_message(cmd, ENDPOINT_OUT, CMD_EM4100ID_READ, NULL, 0);
	send_message(devh, cmd, answer);
	cmd_answer_size = answer[2] - MESSAGE_STRUCTURE_SIZE - 1;	// 1 extra unknow byte
	if (cmd_answer_size < 5)
		fprintf(stdout, "NOTAG\n");
	else
		fprintf(stdout, "%02x%02x%02x%02x%02x\n",answer[5],answer[6],answer[7],answer[8],answer[9]);
};

int send_buzzer(struct libusb_device_handle * devh) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t buzz[1] = {9};
	prepare_message(cmd, ENDPOINT_OUT, CMD_BUZZER, &buzz[0], 1);
	send_message(devh, cmd, answer);
};

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
            default: ;/*print_usage()*/; 
                 exit(EXIT_FAILURE);
        }
    }
    if (argc < 2) {
        //print_usage();
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
        send_read_em4100id(devh);
    }
/*
    if (write_device) {
        send_write_settings(devh, mode, semicolon, questionmark, split, enter);
    }
*/
//	send_buzzer(devh);
    libusb_release_interface(devh, 0); 
out: 
    //	libusb_reset_device(devh); 
    libusb_close(devh); 
    libusb_exit(NULL); 
exit:
    return r >= 0 ? r : -r; 
};
