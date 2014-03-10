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
#define BAUDRATE B9600


void print_usage() {
    printf("p1d_rfid version 1.0, Copyright (c) 2014 Benjamin Larsson <benjamin@southpole.se>\n");  
    printf("Usage: p1d_rfid [OPTION]...\n");
    printf("\t -r Read rfid tag/fob\n");
    printf("\t -f [0|1|2|3|4] output read result in different formats\n");
    printf("\t -l try to detect rfid hardware\n");
    printf("\t -d tty device to connect to (/dev/ttyUSB0 default)\n");
    printf("\t -w [10 char hex string] write data to tag/fob (T55xx tags)\n");
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
    unsigned char id_cmd[] = {'V','E','R','\r'};
    char* device;
    unsigned char firmware_ver[100] = {0};
    int device_found = 0;
    int cnt, i;

    // Send id request
    write(tty_fd_l, id_cmd, 7);
    sleep(1);
    cnt = read(tty_fd_l, &firmware_ver, 100);

    if (cnt >0)
        printf("Found P1D device with firmware version: %s\n", firmware_ver);
    else
        printf("No P1D device found.\n");

    return 0;
}

int send_read(int tty_fd_l, int format) {
    unsigned char st0_cmd[] = {'S','T','0','\r'};
    unsigned char read_cmd[] = {'R','S','D','\r'};
    unsigned char c[100] = {0};
    unsigned char res_arr[100] = {0};
    unsigned char *p, *c1;
    int j, cnt, idx, csum = 0;
	unsigned int bytes[2];
    int retry = 1;

retry1:
    /* Send select EM4100 tag command */
    write(tty_fd_l, st0_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    /* Send read standard data command */
    write(tty_fd_l, read_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    // Check tag present byte
    if ((res_arr[0] == '?') && (res_arr[1] == '1')){
        if (retry) {
            retry = 0;
            goto retry1;
        }
        printf("NOTAG\n");
        return -1;
    }

    /* Convert the hex string to a byte string */
    idx=0;
   	for(j=0 ; j<10 ; j+=2) {
		c1 = &res_arr[j];
		sscanf(c1, "%02x", &bytes[0]);
		c[idx] = bytes[0];
        idx++;
	}

    switch (format) {
        case 0:     // HEX (no spaces)
            res_arr[10]='\0';
            printf("%s",res_arr);
            break;
        case 1:     // HEX (spaces)
            p = res_arr;
            printf("%c%c %c%c %c%c %c%c %c%c",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9]);
            break;
        case 2:     // 8H - 10 decimal
            {
                unsigned int x1 = (c[1] << 24) | (c[2] << 16) | (c[3] << 8) | c[4];
                printf("%010d", x1);
            }
            break;
        case 3:     // 6H - 10 decimal
            {
                unsigned int x1 = (c[2] << 16) | (c[3] << 8) | c[4];
                printf("%010d", x1);
            }
            break;
        case 4:     // (2H + 4H) in decimal
            {
                unsigned int x1 = c[2];
                unsigned int x2 = (c[3] << 8) | c[4];
                printf("%03d,%05d", x1,x2);
            }
            break;
        default: printf("Unknown format: %d\n", format);
    }

    printf("\n");
    return 0;
}

int send_write(int tty_fd_l, char* write_string) {
    unsigned char cmd_array[100] = {0};

    unsigned char st1_cmd[] = {'S','T','1','\r'};
    unsigned char scb_cmd[] = {'S','C','B','\r'};
    unsigned char rtd_cmd[] = {'R','T','D','\r'};
    unsigned char srd_cmd[] = {'S','R','D','\r'};
    unsigned char sra_cmd[] = {'S','R','A','\r'};

    unsigned char c[100] = {0};
    int cnt, i;

    if (strlen(write_string) != 10) {
        printf("Data payload lenght != 10\n");
        return -1;
    }
    /* Send select T55xx tag command */
    write(tty_fd_l, st1_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

    /* Send setup configuration block command */
    write(tty_fd_l, scb_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

    // Check for compatible tag
    if (c[0] == '?'){
        printf("NOTAG\n");
        return -1;
    }

    /* Populate command array */
    cmd_array[0] = 'W';
    cmd_array[1] = 'E';
    cmd_array[2] = 'P';
    memcpy(&cmd_array[3], write_string, 10);
    cmd_array[13] = '\r';
    cmd_array[0] = 'W';

    /* send write command */
    write(tty_fd_l, cmd_array, 16);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

    /** Direct readback does not work for some reason.
     *  So deactivate and then reactivate the reader.
     */

    /* send srd command */
    write(tty_fd_l, srd_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

    /* send sra command */
    write(tty_fd_l, sra_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&c,100);

    send_read(tty_fd_l, 0);
    return 0;
}

int main(int argc,char** argv)
{
    int option = 0;
    int tty_fd, flags;

    char* tty_device = TTYDEV;
    int test_link = 0;
    int read_device = 0;
    int format = 0;
    char* write_string = NULL;

    while ((option = getopt(argc, argv,"d:rlbf:w:")) != -1) {
        switch (option) {
            case 'd' : tty_device = optarg; 
                break;
            case 'l' : test_link = 1;
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
        send_write(tty_fd, write_string);
        close(tty_fd);
        exit(EXIT_SUCCESS);
    }

    if (read_device)
        send_read(tty_fd, format);

exit:
    close(tty_fd);
    return EXIT_SUCCESS;
}

