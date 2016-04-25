/*
Copyright (C) 2015 Benjamin Larsson <benjamin@southpole.se>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


/* lsusb -v

Bus 006 Device 045: ID ffff:0035  
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               2.00
  bDeviceClass            0 (Defined at Interface level)
  bDeviceSubClass         0 
  bDeviceProtocol         0 
  bMaxPacketSize0         8
  idVendor           0xffff 
  idProduct          0x0035 
  bcdDevice            1.00
  iManufacturer           0 
  iProduct                1 USB Reader
  iSerial                 0 
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength           27
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
      bNumEndpoints           0
      bInterfaceClass         3 Human Interface Device
      bInterfaceSubClass      0 No Subclass
      bInterfaceProtocol      0 None
      iInterface              0 
        HID Device Descriptor:
          bLength                 9
          bDescriptorType        33
          bcdHID               1.00
          bCountryCode            0 Not supported
          bNumDescriptors         1
          bDescriptorType        34 Report
          wDescriptorLength      28
          Report Descriptor: (length is 28)
            Item(Global): Usage Page, data= [ 0xa0 0xff ] 65440
                            (null)
            Item(Local ): Usage, data= [ 0x01 ] 1
                            (null)
            Item(Main  ): Collection, data= [ 0x01 ] 1
                            Application
            Item(Local ): Usage, data= [ 0x03 ] 3
                            (null)
            Item(Global): Report ID, data= [ 0x01 ] 1
            Item(Global): Report Size, data= [ 0x08 ] 8
            Item(Global): Report Count, data= [ 0xff ] 255
            Item(Main  ): Feature, data= [ 0x02 ] 2
                            Data Variable Absolute No_Wrap Linear
                            Preferred_State No_Null_Position Non_Volatile Bitfield
            Item(Local ): Usage, data= [ 0x03 ] 3
                            (null)
            Item(Global): Report ID, data= [ 0x02 ] 2
            Item(Global): Report Size, data= [ 0x08 ] 8
            Item(Global): Report Count, data= [ 0xff ] 255
            Item(Main  ): Feature, data= [ 0x02 ] 2
                            Data Variable Absolute No_Wrap Linear
                            Preferred_State No_Null_Position Non_Volatile Bitfield
            Item(Main  ): End Collection, data=none
Device Status:     0x0000
  (Bus Powered)
  
dmesg

usb 6-1.4.4: new low-speed USB device number 45 using ehci-pci
usb 6-1.4.4: New USB device found, idVendor=ffff, idProduct=0035
usb 6-1.4.4: New USB device strings: Mfr=0, Product=1, SerialNumber=0
usb 6-1.4.4: Product: USB Reader
usbhid 6-1.4.4:1.0: couldn't find an input interrupt endpoint

The code is loosly based on the 通讯协议v1.3(IS014443A+B+15693).pdf specification and
https://github.com/Simpleyyt/libfunction_so_usb

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

#define TT_T5577    0x00
#define TT_EM4305   0x02

const static int PACKET_INT_LEN=2;
const static int INTERFACE=0; 
const static int ENDPOINT_INT_IN=0x00; /* endpoint 0x81 address for IN */ 
const static int ENDPOINT_INT_OUT=0x00; /* endpoint 0 address for OUT */ 
const static int TIMEOUT=5000; /* timeout in ms */ 

static int verbose = 0;

static int calc_crc(uint8_t* buf, unsigned int len)
{
    int i;
    uint8_t crc = 0x0;

    for (i=0 ; i<len ; i++) {
        crc ^= buf[i]; 
    }
    return crc;
}

static int handle_packet(uint8_t* pkt, uint8_t* data)
{
    int i;
    uint8_t type, pkt_pl_unk, stx, station_id, pl_len, etx;
    uint8_t cmd, status, bcc, bcc_len, bcc_calc, data_len;
    uint8_t *pl_buf, *bcc_s;

    /* Extract paramaters from packet */
    type       = pkt[0];
    pkt_pl_unk = pkt[6];                // unknown parameter

    stx        = pkt[8];                // start marker
    station_id = pkt[9];
    pl_len     = pkt[10];               // payload length
    etx        = pkt[10+pl_len+1+1];    // end marker
    bcc        = pkt[10+pl_len+1];
    bcc_s      = &pkt[9];
    bcc_len    = 1+1+pl_len;
    data_len   = pl_len - 1;

    /* Calculate payload CRC */
    bcc_calc   = calc_crc(bcc_s, bcc_len);

    if (bcc == bcc_calc) {
        if (verbose) fprintf(stdout, "CRC_OK = 0x%02x\n", bcc);
    } else {
        fprintf(stderr, "CRC_FAIL - BCC=0x%02x | BCC_CALC=0x%02x\n", bcc, bcc_calc);
        return -1;
    }

    switch(type) {
        case IN_PKT:
            if (verbose) fprintf(stdout, "Incoming packet, pkt_pl_unk = 0x%02x\n", pkt_pl_unk);
            break;
        case OUT_PKT:
            if (verbose) fprintf(stdout, "Outgoing packet, pkt_pl_unk = 0x%02x\n", pkt_pl_unk);
            break;
        default:
            fprintf(stderr, "Invalid packet type 0x%02x\n", type);
            return -1;
    }

    if (type == OUT_PKT) {
        if (stx == 0xaa) {
            if (verbose) fprintf(stdout, "STX OK 0x%02x\n", stx);
        } else {
            fprintf(stderr, "STX INVALID! 0x%02x\n", stx);
            return -1;
        }

        if (etx == 0xbb) {
            if (verbose) fprintf(stdout, "ETX OK 0x%02x\n", etx);
        } else {
            fprintf(stderr, "ETX INVALID! 0x%02x\n", etx);
            return -1;
        }

        cmd    = pkt[11];
        pl_buf = &pkt[12];

        switch (cmd) {
            case CMD_GET_SNR:
                if (verbose) fprintf(stdout, "CMD_GET_SNR: 0x%02x 0x%02x\n", pl_buf[0], pl_buf[1]);
                break;
            case CMD_READ:
                fprintf(stdout, "CMD_READ: 0x%02x 0x%02x\n", pl_buf[0], pl_buf[1]);
                break;
            case CMD_WRITE:
                if (verbose) fprintf(stdout, "CMD_WRITE: 0x%02x 0x%02x\n", pl_buf[0], pl_buf[1]);
                break;
            case CMD_LED1:
                fprintf(stdout, "CMD_LED1: 0x%02x 0x%02x\n", pl_buf[0], pl_buf[1]);
                break;
            case CMD_LED2:
                fprintf(stdout, "CMD_LED2: 0x%02x 0x%02x\n", pl_buf[0], pl_buf[1]);
                break;
            case CMD_BUZZER:
                if (verbose) fprintf(stdout, "CMD_BUZZER: 0x%02x 0x%02x\n", pl_buf[0], pl_buf[1]);
                break;
            default:
                fprintf(stderr, "Unknown command: 0x%02x\n", cmd);
                return -1;
        }
    }

    if (type == IN_PKT) {
        if (stx == 0x02) {
            if (verbose) fprintf(stdout, "STX OK 0x%02x\n", stx);
        } else {
            fprintf(stderr, "STX INVALID! 0x%02x\n", stx);
            return -1;
        }

        if (etx == 0x03) {
            if (verbose) fprintf(stdout, "ETX OK 0x%02x\n", etx);
        } else {
            fprintf(stderr, "ETX INVALID! 0x%02x\n", etx);
            return -1;
        }

        status = pkt[11];
        pl_buf = &pkt[12];

        switch (status) {
            case STATUS_OK:
                if (verbose) fprintf(stdout, "CMD_OK\n");
                break;
            case STATUS_FAIL:
                if (verbose) fprintf(stdout, "CMD_FAIL: 0x%02x\n", pl_buf[0]);
                return -1;
                break;
            default:
                fprintf(stderr, "Unknown status: 0x%02x\n", status);
                return -1;
        }

        /* Print data */
        for (i=0 ; i<data_len ; i++) {
            if (data)
                data[i] = pl_buf[i];
            if(i%16 == 0) 
                if (verbose) fprintf(stdout,"\n"); 
            if (verbose) fprintf(stdout, "%02x ", pl_buf[i]); 
        }
        if (verbose) fprintf(stdout, "\n");
    }
    if (verbose) fprintf(stdout, "\n");
    return 0;
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
    pkt[6]  = 0x08;
    pkt[7]  = 0x00;

    /* Populate command payload */
    pkt[8]  = STX_AA;
    pkt[9]  = STATION_ID;
    switch (cmd) {
    case CMD_GET_SNR:
        if (verbose) fprintf(stdout, "CMD_GET_SNR\n");
        pkt[10] = 0x03; // cmd payload size
        pkt[11] = CMD_GET_SNR;
        pkt[12] = 0x00;
        pkt[13] = 0x00;
        pkt[14] = calc_crc(&pkt[9], 1+1+pkt[10]);
        pkt[15] = ETX_BB;
        break;
    case CMD_READ:
        fprintf(stdout, "CMD_READ, not implemented\n");
        break;
    case CMD_WRITE:
        if (verbose) fprintf(stdout, "CMD_WRITE\n");
        pkt[6]  = 0x1f;
        pkt[10] = data_size+1;
        pkt[11] = CMD_WRITE;
        memcpy(&pkt[12], data, data_size);
        pkt[10+data_size+2] = calc_crc(&pkt[9], 1+1+pkt[10]);
        pkt[10+data_size+3] = ETX_BB;
        break;
    case CMD_LED1:
        fprintf(stdout, "CMD_LED1, not implemented\n");
        break;
    case CMD_LED2:
        fprintf(stdout, "CMD_LED2, not implemented\n");
        break;
    case CMD_BUZZER:
        if (verbose) fprintf(stdout, "CMD_BUZZER\n");
        pkt[10] = 0x03; // cmd payload size
        pkt[11] = CMD_BUZZER;
        pkt[12] = 0x01; // * 20ms = buzzer time
        pkt[13] = 0x01; // buzzer cycle time
        pkt[14] = calc_crc(&pkt[9], 1+1+pkt[10]);
        pkt[15] = ETX_BB;
        break;
    default:
        fprintf(stderr, "Unknown comamnd: 0x%02x\n", cmd);
        return -1;
    }
};

static void prepare_write_buffer(uint8_t* db, uint8_t* rn, int tag_type)
{
    db[0]  = 0x00;  // write mode control ???
    db[1]  = 0x01;  // length ???
    db[2]  = 0x01;  // start address ???
    db[3]  = tag_type;
    db[4]  = rn[0]; /* EM4100 tag number */
    db[5]  = rn[1]; /* EM4100 tag number */
    db[6]  = rn[2]; /* EM4100 tag number */
    db[7]  = rn[3]; /* EM4100 tag number */
    db[8]  = rn[4]; /* EM4100 tag number */
    db[9]  = 0x80;  // unknown
}

static int send_read(struct libusb_device_handle *devh, int format) {
    uint8_t out_pkt[PACKET_CTRL_LEN] = {0};
    uint8_t answer[PACKET_CTRL_LEN] = {0};
    uint8_t c[6] = {0};
    int r, i;

    /* Prepare read packet */
    create_out_packet(out_pkt, CMD_GET_SNR, NULL, 0);
    handle_packet(out_pkt, NULL);

    /* Send read packet */
    r = libusb_control_transfer(devh,CTRL_OUT,HID_SET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x00,0,out_pkt, PACKET_CTRL_LEN,TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control Out error %d\n", r); 
        return r; 
    }
    /* Get answer */
    r = libusb_control_transfer(devh,CTRL_IN,HID_GET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x00,0, answer,PACKET_CTRL_LEN, TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control IN error %d\n", r); 
        return r; 
    }
    /* Handle answer */
    r = handle_packet(answer, c);
    if (r < 0) {
        fprintf(stdout, "NOTAG\n");
        return 0;
    }

    switch (format) {
        case 0:     // HEX (no spaces)
            for (i=0 ; i<5 ; i++) {
                fprintf(stdout,"%02X",c[i]);
            }
            break;
        case 1:     // HEX (spaces)
            fprintf(stdout,"%02X",c[0]);
            for (i=1 ; i<5 ; i++) {
                fprintf(stdout," %02X",c[i]);
            }
            break;
        case 2:     // 8H - 10 decimal
            {
                unsigned int x1 = (c[1] << 24) | (c[2] << 16) | (c[3] << 8) | c[4];
                fprintf(stdout,"%010d", x1);
            }
            break;
        case 3:     // 6H - 10 decimal
            {
                unsigned int x1 = (c[2] << 16) | (c[3] << 8) | c[4];
                fprintf(stdout,"%010d", x1);
            }
            break;
        case 4:     // (2H + 4H) in decimal
            {
                unsigned int x1 = c[2];
                unsigned int x2 = (c[3] << 8) | c[4];
                fprintf(stdout,"%03d,%05d", x1,x2);
            }
            break;
        default: fprintf(stderr,"Unknown format: %d\n", format);
    }

    fprintf(stdout,"\n");
    return 0;
}

static int send_write(struct libusb_device_handle *devh, uint8_t* rfid_string, int tag_format) {
    uint8_t write_buffer[WRITE_BUF_SIZE] = {0};
    uint8_t out_pkt[PACKET_CTRL_LEN] = {0};
    uint8_t answer[PACKET_CTRL_LEN] = {0};
    int i, r, idx=0;
    unsigned int bytes[2];
    uint8_t rfid_hex[8];
    uint8_t* c1;

    /* Convert the hex string to a byte array */
    for(i=0 ; i<10 ; i+=2) {
        c1 = &rfid_string[i];
        sscanf(c1, "%02x", &bytes[0]);
        rfid_hex[idx] = bytes[0];
        idx++;
    }

    /* Prepare write packet */
    if (verbose)  printf("Testing write packet code\n");
    prepare_write_buffer(write_buffer, rfid_hex, tag_format);
    create_out_packet(out_pkt, CMD_WRITE, write_buffer, WRITE_BUF_SIZE);
    handle_packet(out_pkt, NULL);

    /* Dump packet */
/*    for (i=0 ; i<48 ; i++) {
        if(i%16 == 0) 
            fprintf(stdout,"\n");
        fprintf(stdout, "%02x ", out_pkt[i]); 
    }
    fprintf(stdout,"\n");
*/
    /* Send write packet */
    r = libusb_control_transfer(devh,CTRL_OUT,HID_SET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x00,0,out_pkt, PACKET_CTRL_LEN,TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control Out error %d\n", r); 
        return r; 
    }

    /* Get answer */
    r = libusb_control_transfer(devh,CTRL_IN,HID_GET_REPORT,(HID_REPORT_TYPE_FEATURE<<8)|0x00,0, answer,PACKET_CTRL_LEN, TIMEOUT); 
    if (r < 0) { 
        fprintf(stderr, "Control IN error %d\n", r); 
        return r; 
    }

    /* Handle answer, writes seem to always succeed */
    r = handle_packet(answer, NULL);
    if (r < 0) {
        //fprintf(stdout, "NOTAG\n");
        return 0;
    }
    sleep(1);

    /* T5577 tags seem to need to be removed from the field before you can query them again */
    if (tag_format==TT_T5577)
        fprintf(stdout, "OK\n");
    else
        send_read(devh, 0);
}

static void print_usage() {
    fprintf(stdout,"idrw_linux version 1.0, Copyright (c) 2015 Benjamin Larsson <benjamin@southpole.se>\n");     
    fprintf(stdout,"Usage: rfid_app [OPTION]...\n");
    fprintf(stdout,"\t -r Read rfid tag/fob\n");
    fprintf(stdout,"\t -f [0|1|2|3|4] output read result in different formats\n");
    fprintf(stdout,"\t -w [10 char hex string] write data to tag/fob, add -f 0 for T5577 and -f 2 for EM4305\n");
    fprintf(stdout,"\t -v verbose/debug output\n\n");
    fprintf(stdout,"\t example: idrw_linux -f 2 -w 0F00223C29 for writing EM4305 tag\n");
    fprintf(stdout,"\t example: idrw_linux -f 0 -w 0F00223C29 for writing T5577 tag\n\n");
    fprintf(stdout,"\t If the device blinks and fail to work run \"sudo lsusb -v\"\n");
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
    if (!devh) { 
        if (verbose) fprintf(stderr, "Could not find/open LVR Generic HID device\n"); 
        goto out; 
    } 
    if (verbose) fprintf(stdout, "Successfully found the RFID R/W device\n"); 

    r = libusb_set_configuration(devh, 1); 
    if (r < 0) { 
        fprintf(stderr, "libusb_set_configuration error %d\n", r); 
        goto out; 
    } 

    r = libusb_claim_interface(devh, 0); 
    if (r < 0) { 
        fprintf(stderr, "libusb_claim_interface error %d\n", r); 
        goto out; 
    }

    // addy experimental - wakeup device as per lsusb -v
    libusb_device *dev;
    dev=  libusb_get_device(devh);
    struct libusb_config_descriptor *config;
    r = libusb_get_config_descriptor(dev, 0, &config);
    if (r < 0) {
        fprintf(stderr, "libusb_get_config_descriptor error %d\n", r);
        goto out;
    }
    char tmp[1];
    r = libusb_control_transfer(devh,LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE,LIBUSB_REQUEST_GET_DESCRIPTOR,(LIBUSB_DT_REPORT << 8),0,tmp,1,TIMEOUT);
    libusb_reset_device(devh);
    sleep(2);

    /* USB init done */


    /* Handle io */

    if (write_string) {
        send_write(devh, write_string, format);
    }

    if (read_device) {
        send_read(devh, format);
    }

    libusb_release_interface(devh, 0); 
out: 
    //	libusb_reset_device(devh); 
    libusb_close(devh); 
    libusb_exit(NULL); 
exit:
    return r >= 0 ? r : -r; 
} 
