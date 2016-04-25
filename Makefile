all: rfid_app idrw_linux p1d_rfid

rfid_app: rfid_app.c 
	gcc -o rfid_app rfid_app.c

idrw_linux: idrw_linux.c
	gcc -o idrw_linux idrw_linux.c -I/usr/local/include -L. -lnsl -lm -lc -L/usr/local/lib -lusb-1.0

p1d_rfid: p1d_rfid.c
	gcc p1d_rfid.c -o p1d_rfid

clean:
	rm -f *.o rfid_app idrw_linux p1d_rfid

install:
	cp 20-rwrfid.rules /etc/udev/rules.d/

