/*
Copyright (C) 2014 Benjamin Larsson <benjamin@southpole.se>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <getopt.h>

#define TTYDEV "/dev/ttyUSB0"
#define BAUDRATE B38400

/** TODO
 *  Add better response parsing
 *  Handle missing 
 *  Factorize code
 *  Add locking support
 *  Add send beep command line option
 *  Factorize out the checksum routine
 *  Minimize read delay
 **/


void print_usage() {
    printf("rfid_app version 1.0, Copyright (c) 2014 Benjamin Larsson <benjamin@southpole.se>\n");     
    printf("Usage: rfid_app [OPTION]...\n");
    printf("\t -r Read rfid tag/fob\n");
    printf("\t -f [0|1|2|3|4] output read result in different formats\n");
    printf("\t -l try to detect rfid hardware\n");
    printf("\t -d tty device to connect to (/dev/ttyUSB0 default)\n");
    printf("\t -w [10 char hex string] write data to tag/fob\n");
    printf("\t -b don't beep while accessing tag/fob, might affect some tags (T55xx/EM4305) tags\n");
}

int open_device(char* tty_device) {
    struct termios tio;
    struct termios stdio;
    struct termios old_stdio;
    int tty_fd_l;

    memset(&stdio,0,sizeof(stdio));
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;
    if((tty_fd_l = open(tty_device , O_RDWR | O_NONBLOCK)) == -1){
        printf("Error while opening %s\n", tty_device); // Just if you want user interface error control
        exit(EXIT_FAILURE);
    }
    cfsetospeed(&tio,BAUDRATE);    
    cfsetispeed(&tio,BAUDRATE);            // baudrate is declarated above
    tcsetattr(tty_fd_l,TCSANOW,&tio);

    return tty_fd_l;
}

int get_id(int tty_fd_l) {
    unsigned char id_cmd[] = {0xAA,0xDD,0x00,0x03,0x01,0x02,0x03};
    char* device;
    unsigned char c[100] = {0};
    int device_found = 0;
    int cnt, i;

    // Send id request
    write(tty_fd_l, id_cmd, 7);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);
    device = &c[7];

    if (!strcmp(device,"ID card reader & writer6"))
        printf("Found device: %s\n", device);
    else
        printf("Unknown or no device found: %s\n", device);

    return 0;
}

int send_beep(int tty_fd_l) {
    unsigned char beep_cmd[] = {0xAA,0xDD,0x00,0x04,0x01,0x03,0x0A,0x08};
    unsigned char c[100] = {0};
    int cnt;

    write(tty_fd_l, beep_cmd, 8);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);
}

int send_read(int tty_fd_l, int beep, int format) {
    unsigned char read_cmd[] = {0xAA,0xDD,0x00,0x03,0x01,0x0C,0x0D};
    unsigned char c[100] = {0};
    unsigned char res_arr[100] = {0};
    int i, cnt, idx, csum = 0;


    if (beep)
        send_beep(tty_fd_l);

    write(tty_fd_l, read_cmd, 7);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    // Check tag present byte
    if (res_arr[6] == 0x01) {
        printf("NOTAG\n");
        return -1;
    }


    // Calculate checksum (xor over byte 5 to end-1)

    for (i=4 ; i<cnt-1 ; i++) {
        csum^=c[i];
    }

    // Remove 0xAA,0x00 patterns
    idx = 7;
    for (i=7 ; i<cnt-1 ; i++) {
        c[i] = res_arr[idx];
        if ((res_arr[idx]==0xAA) && (res_arr[idx+1]==0x00)) {
            idx++;
        }
        idx++;
    }

    if (csum != c[cnt-1])
        printf("Read checksum missmatch!: %X vs %X\n", csum, c[cnt]);

    switch (format) {
        case 0:     // HEX (no spaces)
            for (i=7 ; i<12 ; i++) {
                printf("%02X",c[i]);
            }
            break;
        case 1:     // HEX (spaces)
            printf("%02X",c[7]);
            for (i=8 ; i<12 ; i++) {
                printf(" %02X",c[i]);
            }
            break;
        case 2:     // 8H - 10 decimal
            {
                unsigned int x1 = (c[8] << 24) | (c[9] << 16) | (c[10] << 8) | c[11];
                printf("%010d", x1);
            }
            break;
        case 3:     // 6H - 10 decimal
            {
                unsigned int x1 = (c[9] << 16) | (c[10] << 8) | c[11];
                printf("%010d", x1);
            }
            break;
        case 4:     // (2H + 4H) in decimal
            {
                unsigned int x1 = c[9];
                unsigned int x2 = (c[10] << 8) | c[11];
                printf("%03d,%05d", x1,x2);
                break;
            }
        case 5:     // (5H) in decimal but remove 10th digit
            {
                unsigned long long int x2 = c[7];
                unsigned long long int x1 = ((x2 << 32) | (c[8] << 24) | (c[9] << 16) | (c[10] << 8) | c[11]);
                printf("%09llu", x1);
                break;
            }
        case 6:
            {
                unsigned char num_buf[11] = {0};
                unsigned char* num_buf_ptr = num_buf + 1;
                unsigned long long int x2 = c[7];
                unsigned long long int x1 = ((x2 << 32) | (c[8] << 24) | (c[9] << 16) | (c[10] << 8) | c[11]);
                snprintf(num_buf, 11, "%09llu", x1);
                printf("%s", num_buf_ptr);
                break;
            }

        default: printf("Unknown format: %d\n", format);
    }

    printf("\n");
    return 0;
}

int send_write(int tty_fd_l, int beep, char* write_string) {
    unsigned char read_cmd[] = {0xAA,0xDD,0x00,0x03,0x01,0x0C,0x0D};
//    unsigned char write_cmd[] = {0xAA,0xDD,0x00,0x09, 0x02,0x0C,0x00, 0x01,0x04,0x6E,0x87,0xA9, 0x4B};
    unsigned char cmd_array[100] = {0};
    unsigned char c[100] = {0};
    int i,j, cnt, csum = 0, idx = 0;
	unsigned char* c1;
	unsigned int bytes[2];
    unsigned char cmd_payload[8];

    if (strlen(write_string) != 10) {
        printf("Data payload lenght != 10\n");
        return -1;
    }

    if (beep)
        send_beep(tty_fd_l);

    // Populate command array
    cmd_array[0] = 0xAA;
    cmd_array[1] = 0xDD;
    cmd_array[2] = 0x00;
    cmd_array[3] = 0x09;

    // prepare cmd_payload buffer
    cmd_payload[0] = 0x03;
    cmd_payload[1] = 0x0C;
    cmd_payload[2] = 0x00;
    idx = 3;    

    // Convert the hex string to a byte string and place them in the cmd_payload buffer
   	for(j=0 ; j<10 ; j+=2) {
		c1 = &write_string[j];
		sscanf(c1, "%02x", &bytes[0]);
		cmd_payload[idx] = bytes[0];
        idx++;
	}

    // Calculate checksum and insert in the cmd_array
    idx=0;
    csum=0;
    for (i=0 ; i<8 ; i++) {
        csum^=cmd_payload[i];
        if (cmd_payload[i] == 0xAA) {
            // ensure the byte stream never has data with 0xAA,0xDD
            // by inserting an extra 0xAA when a 0xAA is found
            cmd_array[4+idx] = 0xAA;
            idx++;
        }
        cmd_array[4+idx] = cmd_payload[i];
        // printf("%02X - > %02X\n", cmd_array[4+idx], csum);
        idx++;
    }
    cmd_array[4+idx] = csum;
    if (csum == 0xAA) {
        idx++;
        cmd_array[4+idx] = 0xAA;
    }


/*
    for (i=0 ; i<4+idx+1 ; i++) {
        printf("%02X ", cmd_array[i]);
    }
    printf("\n");
    printf("idx=%d\n",idx+4);
*/
    // send first write command
    write(tty_fd_l, cmd_array, idx+5);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

/*    printf("cnt = %d\n", cnt);
    for (i=0 ; i<cnt ; i++) {
        printf("%02X ", c[i]);
    }
    printf("\n");
*/


    // send write read command
/*    write(tty_fd_l, read_cmd, 7);
    sleep(1);
    cnt = read(tty_fd_l,&c,100); 

    printf("cnt = %d\n", cnt);
    for (i=0 ; i<cnt ; i++) {
        printf("%02X ", c[i]);
    }
    printf("\n");
*/
    // send second write command
    cmd_payload[0] = 0x02;

    // Calculate checksum and insert in the cmd_array
    idx=0;
    csum=0;
    for (i=0 ; i<8 ; i++) {
        csum^=cmd_payload[i];
        if (cmd_payload[i] == 0xAA) {
            // ensure the byte stream never has data with 0xAA,0xDD
            // by inserting an extra 0xAA when a 0xAA is found
            cmd_array[4+idx] = 0xAA;
            idx++;
        }
        cmd_array[4+idx] = cmd_payload[i];
//        printf("%02X - > %02X\n", cmd_array[4+idx], csum);
        idx++;
    }
    cmd_array[4+idx] = csum;
    if (csum == 0xAA) {
        idx++;
        cmd_array[4+idx] = 0xAA;
    }


/*
    for (i=0 ; i<4+idx+1 ; i++) {
        printf("%02X ", cmd_array[i]);
    }
    printf("\n");
*/
    // send second write command
    write(tty_fd_l, cmd_array, idx+5);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

/*
    // Check if command failed
    if (cnt == -1) {
        printf("Quirk detected\n");
        //failed, then apply hardware quirk workaround
        cmd_array[14] = cmd_array[13];
        cmd_array[13] = cmd_array[12];
        write(tty_fd_l, cmd_array, 14);
        sleep(1);
        cnt = read(tty_fd_l,&c,100);
    }
*/

    send_read(tty_fd_l, 0, 0);
}

int main(int argc,char** argv)
{
    int option = 0;
    int tty_fd, flags;

    char* tty_device = TTYDEV;
    int test_link = 0;
    int beep = 1;
    int read_device = 0;
    int format = 0;
    char* write_string = NULL;

    while ((option = getopt(argc, argv,"d:rlbf:w:")) != -1) {
        switch (option) {
            case 'd' : tty_device = optarg; 
                break;
            case 'l' : test_link = 1;
                break;
            case 'b' : beep = 0;
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

    tty_fd = open_device(tty_device);

    if (test_link)
        get_id(tty_fd);

    if (write_string) {
        send_write(tty_fd, beep, write_string);
        close(tty_fd);
        exit(EXIT_SUCCESS);
    }

    if (read_device)
        send_read(tty_fd, beep, format);

exit:
    close(tty_fd);
    return EXIT_SUCCESS;
}

