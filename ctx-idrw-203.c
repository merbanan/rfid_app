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
0x13	Em4305login		(7 bytes): first byte is 4 bit cmd, 3 for login
0x13	Em4305disable	(7 bytes): unknown layout
0x13	Em4305readword	(7 bytes): unknown layout
0x13	Em4305writeword	(7 bytes): unknown layout
0x13	Em4305protect	(7 bytes): unknown layout
0x14	CarrierOff		(1 bytes): 2, turn off 125kHz carrier
0x14	CarrierOn		(1 bytes): 3, turn on 125kHz carrier


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
#include <malloc.h>
#include <libusb-1.0/libusb.h> 

#define AUTO_FORMAT 0
#define T5577_FORMAT 1
#define EM4305_FORMAT 2

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
#define CMD_T5557_BLOCK_WRITE	0x12
#define CMD_EM4305_CMD	0x13

/* Commands (to computer) */
#define CMD_EM4100ID_ANSWER	0x90
static int verbose = 0;
static int handle_events = 0;

static int timeout=1000; /* timeout in ms */

int prepare_message(uint8_t *out_buf, int endpoint, int command, uint8_t *pl_buf, int pl_buf_size) {

	int i,j;
	int x = 0;
	memset(out_buf, 0, 24);
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
//usleep(20000);
	r = libusb_interrupt_transfer(devh, ENDPOINT_IN, answer, 48, &bt, timeout);
	r = libusb_interrupt_transfer(devh, ENDPOINT_OUT, message, 24, &bt, timeout);
//	usleep(50000);

    if (verbose) fprintf(stdout, "Answer:\n");
    for (i=0 ; i<48 ; i++) {
        if(i%16 == 0)
            if (verbose) fprintf(stdout,"\n");
        if (verbose) fprintf(stdout, "%02x ", answer[i]);
    }
    if (verbose) fprintf(stdout, "\n");

};


void interrupt_cb(struct libusb_transfer *xfr)
{
	int i;
	uint8_t* answer;

    switch(xfr->status)
    {
        case LIBUSB_TRANSFER_COMPLETED:
			 handle_events-=1;
			answer = xfr->buffer;
			fprintf(stdout, "interrupt transfer actual_length: %d ", xfr->actual_length);
			if (xfr->actual_length == 48)
				for (i=0 ; i<24 ; i++) {
					if(i%16 == 0)
						if (verbose) fprintf(stdout,"\n");
					if (verbose) fprintf(stdout, "%02x ", answer[i]);
				}
				if (verbose) fprintf(stdout, "\n");

//				fprintf(stdout, "%02x%02x%02x%02x%02x\n",answer[5],answer[6],answer[7],answer[8],answer[9]);
			else
				fprintf(stdout, "\n");
            break;
        case LIBUSB_TRANSFER_CANCELLED:
        case LIBUSB_TRANSFER_NO_DEVICE:
        case LIBUSB_TRANSFER_TIMED_OUT:
        case LIBUSB_TRANSFER_ERROR:
        case LIBUSB_TRANSFER_STALL:
        case LIBUSB_TRANSFER_OVERFLOW:
if (verbose) fprintf(stdout, "transfer error\n");
            break;
    }
}

int init_protocol(struct libusb_device_handle * devh) {

	// the protocol needs a previous sent interrupt in request
	// and it should not be handled
	// most likely to sync the device buffer somehow
	uint8_t* message;
	struct libusb_transfer *xfr_in;
	message = calloc(1, 48);
	xfr_in = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(xfr_in, devh, ENDPOINT_IN, message, 48,
		                      interrupt_cb, NULL, 0);

	if(libusb_submit_transfer(xfr_in) < 0)
		libusb_free_transfer(xfr_in);
	else
		if (verbose) fprintf(stdout, "init succeeded\n");
		
	usleep(500 *1000);
}

int uninit_protocol(struct libusb_device_handle * devh) {

if (verbose) fprintf(stdout, "uninit_protocol\n");

	while(handle_events) {
		if(libusb_handle_events(NULL) != LIBUSB_SUCCESS) break;
		if (verbose) fprintf(stdout, "loop uninit\n");
	}


}

int send_message_async(struct libusb_device_handle * devh, uint8_t *message, uint8_t *answer) {
	int r,i;
	int bt = 0;
	struct libusb_transfer *xfr_out, *xfr_in;
	uint8_t* usb_msg_out = malloc(24);
	uint8_t* usb_msg_in = calloc(1, 48);

	xfr_out = libusb_alloc_transfer(0);
	xfr_in = libusb_alloc_transfer(0);

	memcpy(usb_msg_out, message, 24);
	libusb_fill_interrupt_transfer(xfr_out, devh, ENDPOINT_OUT, usb_msg_out, 24,
		                      interrupt_cb, NULL, timeout);

	libusb_fill_interrupt_transfer(xfr_in, devh, ENDPOINT_IN, usb_msg_in, 48,
		                      interrupt_cb, NULL, 0);


	if(libusb_submit_transfer(xfr_out) < 0)
		libusb_free_transfer(xfr_out);
	handle_events = 2;
	usleep(50 * 1000);

	while(handle_events) {
		if(libusb_handle_events(NULL) != LIBUSB_SUCCESS) break;
		if (verbose) fprintf(stdout, "event %d handled\n", handle_events);
	}

	usleep(100 * 1000);


	handle_events = 1;
	if(libusb_submit_transfer(xfr_in) < 0)
		libusb_free_transfer(xfr_in);

	usleep(50 * 1000);

	if (verbose) fprintf(stdout, "here3\n");
	return 0;
};


int send_read_em4100id(struct libusb_device_handle * devh) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	int cmd_answer_size = 0;

	prepare_message(cmd, ENDPOINT_OUT, CMD_EM4100ID_READ, NULL, 0);
	send_message_async(devh, cmd, answer);
	return 0;
	cmd_answer_size = answer[2] - MESSAGE_STRUCTURE_SIZE - 1;	// 1 extra unknow byte
	if (cmd_answer_size < 5)
		fprintf(stdout, "NOTAG\n");
	else
		fprintf(stdout, "%02x%02x%02x%02x%02x\n",answer[5],answer[6],answer[7],answer[8],answer[9]);

	return 0;
};

int t55xx_reset(struct libusb_device_handle * devh) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t reset[5] = {0x0, 0x0, 0x0, 0x0, 0x0};

	prepare_message(cmd, ENDPOINT_OUT, CMD_T5557_BLOCK_WRITE, reset, 5);
	send_message_async(devh, cmd, answer);
	
};

int t55xx_block_write(struct libusb_device_handle * devh, int block, uint8_t* data_buf, int data_buf_size, uint8_t *password) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t bw_buf[7] = {0};
	int cmd_answer_size = 0;

	
	bw_buf[0] = 0x04; //??
	bw_buf[1] = 0x00; //??
	bw_buf[2] = data_buf[0];
	bw_buf[3] = data_buf[1];
	bw_buf[4] = data_buf[2];
	bw_buf[5] = data_buf[3];
	bw_buf[6] = block;


	prepare_message(cmd, ENDPOINT_OUT, CMD_T5557_BLOCK_WRITE, bw_buf, 7);
	send_message_async(devh, cmd, answer);


/* 	SS PP 11 22 33 44 BB

	SS	Subcommand	(4 for write)	or matches t5557 specs
	PP	Protection bit
	11	8 first bits
	22	8 second bits
	33  8 third bits
	44	8 forth bits
	BB	block number
*/
/*
0000   03 01 0c 12 04 00 ff 80 60 28 01 2d 04 00 00 00
0000   03 01 0c 12 04 00 0c 04 81 42 02 d2 04 00 00 00

0        1        2        3        4        5        6        7
ff       80       60       28       0c       04       81       42
11111111 10000000 01100000 00101000 00001100 00000100 10000001 01000010
HHHHHHHH H0000011 11122222 33333444 44555556 66667777 78888899 999SSSSS
00000000 01111111 11122222 22222333 33333334 44445555 55555566 66666666
          0000X00 00X1111X 1111X222 2X2222X3 333X3333 X4444X44 44XCCCCS
111111111
00000
00011	1
00000
00101	2
00000
00110	3
00000
01001	4
00000
01010	5
00010
*/
};

int em4100_column_parity(uint8_t* hex_buf, int shift) {
	int i, p=0;

	for (i=0 ; i<5 ; i++) {
		p += (hex_buf[i] >> shift+4) &1;
		p += (hex_buf[i] >> shift) &1;
	}
	return p&1;
}

int hex_to_em4100_layout(uint8_t* hex_buf, uint8_t* out_buf) {
	int p0,p1,p2,p3,p4,p5,p6,p7,p8,p9;
	int pc0,pc1,pc2,pc3;
	uint8_t ep[16] = {0,1,1,0, 1,0,0,1, 1,0,0,1, 0,1,1,0};

	p0 = ep[hex_buf[0]  >> 4];
	p1 = ep[hex_buf[0] & 0xf];
	p2 = ep[hex_buf[1]  >> 4];
	p3 = ep[hex_buf[1] & 0xf];
	p4 = ep[hex_buf[2]  >> 4];
	p5 = ep[hex_buf[2] & 0xf];
	p6 = ep[hex_buf[3]  >> 4];
	p7 = ep[hex_buf[3] & 0xf];
	p8 = ep[hex_buf[4]  >> 4];
	p9 = ep[hex_buf[4] & 0xf];

	pc0 = em4100_column_parity(hex_buf, 3);
	pc1 = em4100_column_parity(hex_buf, 2);
	pc2 = em4100_column_parity(hex_buf, 1);
	pc3 = em4100_column_parity(hex_buf, 0);

	out_buf[0] = 0xff;
	out_buf[1] = 0x80 | ((hex_buf[0]>>1)&0x78) | (p0<<2) | ((hex_buf[0]>>2)&0x03);
	out_buf[2] = (hex_buf[0]<<6) | (p1<<5) | (hex_buf[1]>>3) | p2;
	out_buf[3] = (hex_buf[1]<<4) | (p3<<3) | (hex_buf[2]>>5);
	out_buf[4] = ((hex_buf[2]<<3)&0x80) | (p4<<6) | ((hex_buf[2]<<2)&0x3c) | (p5<<1) | (hex_buf[3]>>7);
	out_buf[5] = ((hex_buf[3]<<1)&0xe0) | (p6<<4) | (hex_buf[3]&0xf);
	out_buf[6] = (p7<<7) | ((hex_buf[4]>>1)&0x78) | (p8<<2) | ((hex_buf[4]>>2)&0x03);
	out_buf[7] = (hex_buf[4]<<6) | (p9<<6) | (pc0<<4) | (pc1<<3) | (pc2<<2) | (pc3<<1); 
};

int em4305_write_word(struct libusb_device_handle * devh, int word, uint8_t* data_buf, int data_buf_size, uint8_t *password) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t ww_buf[7] = {0};
	int cmd_answer_size = 0;

	ww_buf[0] = 0x01; //write command
	ww_buf[1] = word; //word index
	ww_buf[2] = data_buf[0];
	ww_buf[3] = data_buf[1];
	ww_buf[4] = data_buf[2];
	ww_buf[5] = data_buf[3];
	ww_buf[6] = 0x0;  // ??

	prepare_message(cmd, ENDPOINT_OUT, CMD_EM4305_CMD, ww_buf, 7);
	send_message_async(devh, cmd, answer);
};


int em4305_login(struct libusb_device_handle * devh) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t login[7] = {0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	prepare_message(cmd, ENDPOINT_OUT, CMD_EM4305_CMD, login, 7);
	send_message_async(devh, cmd, answer);
}


int send_write_em4100id_em4305(struct libusb_device_handle * devh, uint8_t *hex_buf) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t ds[8] = {0};
	uint8_t em4100_config[4] = {0xfa, 0x01, 0x80, 0x00};
	int cmd_answer_size = 0;

	fprintf(stdout, "%02x%02x%02x%02x%02x\n",hex_buf[0],hex_buf[1],hex_buf[2],hex_buf[3],hex_buf[4]);
	hex_to_em4100_layout(hex_buf, ds);
	fprintf(stdout, "%02x %02x %02x %02x ",  ds[0],ds[1],ds[2],ds[3]);
	fprintf(stdout, "%02x %02x %02x %02x\n",ds[4],ds[5],ds[6],ds[7]);

	/* login to em4305 tag */
	em4305_login(devh);

	/* write em4100 bitstream to word 5 and 6 */
	em4305_write_word(devh, 5, ds, 4, NULL);
	em4305_write_word(devh, 6, &ds[4], 4, NULL);

	/* write em4305 configuration word (4) */
	em4305_write_word(devh, 4, em4100_config, 4, NULL);

};

int send_write_em4100id(struct libusb_device_handle * devh, uint8_t *hex_buf, int format) {
	uint8_t cmd[24] = {0};
	uint8_t answer[48] = {0};
	uint8_t ds[8] = {0};
	uint8_t em4100_config_t5577[4] = {0x00, 0x14, 0x80, 0x41};
	uint8_t em4100_config_em4305[4] = {0xfa, 0x01, 0x80, 0x00};
	int cmd_answer_size = 0;

	fprintf(stdout, "%02x%02x%02x%02x%02x\n",hex_buf[0],hex_buf[1],hex_buf[2],hex_buf[3],hex_buf[4]);
	hex_to_em4100_layout(hex_buf, ds);
	fprintf(stdout, "%02x %02x %02x %02x ",  ds[0],ds[1],ds[2],ds[3]);
	fprintf(stdout, "%02x %02x %02x %02x\n",ds[4],ds[5],ds[6],ds[7]);

	if (format == AUTO_FORMAT) {
		fprintf(stdout, "Autoformat not supported yet!\n");
		return 1;
	} else if (format == T5577_FORMAT) {
		/* write em4100 bitstream to block 1 and 2 */
		t55xx_block_write(devh, 1, ds, 4, NULL);
		t55xx_block_write(devh, 2, &ds[4], 4, NULL);

		//0000   03 01 0c 12 04 00 00 14 80 41 00 ce 04

		/* write configuration in block 0 to emulate EM4100; RF/64, Manchester, max block = 2 */
		t55xx_block_write(devh, 0, em4100_config_t5577, 4, NULL);

		/* reset tag */
		t55xx_reset(devh);

		//todo cycle the field 0x14
	} else if (format == EM4305_FORMAT){
		/* login to em4305 tag */
		em4305_login(devh);

		/* write em4100 bitstream to word 5 and 6 */
		em4305_write_word(devh, 5, ds, 4, NULL);
		em4305_write_word(devh, 6, &ds[4], 4, NULL);

		/* write em4305 configuration word (4) */
		em4305_write_word(devh, 4, em4100_config_em4305, 4, NULL);
		//todo cycle the field 0x14
	} else {
		fprintf(stdout, "Unknown format!\n");
		return 1;
	}

	return 0;
};



int send_buzzer(struct libusb_device_handle * devh, uint8_t duration) {
	uint8_t cmd[48] = {0};
	uint8_t answer[48] = {0};
	uint8_t buzz[1] = {9};
	buzz[1] = duration;
	prepare_message(cmd, ENDPOINT_OUT, CMD_BUZZER, &buzz[0], 1);
	send_message_async(devh, cmd, answer);
};

int hex_string_to_bytes(uint8_t *hex_string, uint8_t *byte_array) {
	uint8_t* c1;
	int i,idx=0;
	unsigned int bytes[2];
	for(i=0 ; i<10 ; i+=2) {
	    c1 = &hex_string[i];
	    sscanf(c1, "%02x", &bytes[0]);
	    byte_array[idx] = bytes[0];
	    idx++;
	}
}

int main(int argc,char** argv)
{ 
    int r = 1, i;
    struct libusb_device_handle *devh = NULL; 

    int option = 0;
    int read_device = 0;
    int write_device = 0;
	int format = AUTO_FORMAT;
	int buzzer = 0;
    int semicolon=0, questionmark=0, split=0, enter=0;
    char* write_string = NULL;

    while ((option = getopt(argc, argv,"w:vrb:sqlef:")) != -1) {
        switch (option) {
            case 'v' : verbose = 1;
                break;
            case 'r' : read_device = 1;
                break;
            case 'b' : buzzer = optarg[0]+'0';
                break;
            case 's' : semicolon = 1;
                break;
            case 'q' : questionmark = 1;
                break;
            case 'l' : split = 1;
                break;
            case 'e' : enter = 1;
                break;
			case 'f' : format = atoi(optarg);
				break;
            case 'w' : write_string = optarg;
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
    if (r < 0 && r != LIBUSB_ERROR_NOT_FOUND) { 
        fprintf(stderr, "libusb_detach_kernel_driver error %d\n", r); 
        goto out; 
    }


    r = libusb_claim_interface(devh, 0); 
    if (r < 0) { 
        fprintf(stderr, "libusb_claim_interface error %d\n", r); 
        goto out; 
    }

	init_protocol(devh);

    if (buzzer) {
		send_buzzer(devh, 9);
    }


    if (read_device) {
//		send_buzzer(devh);
        send_read_em4100id(devh);
        send_read_em4100id(devh);
        send_read_em4100id(devh);
        send_read_em4100id(devh);
        send_read_em4100id(devh);
    }

    if (write_string) {
		uint8_t hex_buf[5];
		hex_string_to_bytes(write_string, hex_buf);
		send_write_em4100id(devh, hex_buf, format);
//		send_write_em4100id_em4305(devh, hex_buf);
    }

if (verbose) fprintf(stdout, "uninit\n");

//	uninit_protocol(devh);
//	send_buzzer(devh);
    libusb_release_interface(devh, 0); 
out: 
    //	libusb_reset_device(devh); 
    libusb_close(devh); 
    libusb_exit(NULL); 
exit:
    return r >= 0 ? r : -r; 
};
