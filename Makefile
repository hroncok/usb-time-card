CC=gcc
CFLAGS=-Wall -pedantic -g
LIBS=-ludev -lconfig

usb-time-card-deamon: usb-time-card-deamon.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

all: usb-time-card-deamon

root: usb-time-card-deamon usb-time-card usb-time-card.conf
	mkdir -p root/usr/bin
	mkdir -p root/usr/src
	mkdir -p root/etc/init.d
	cp usb-time-card root/etc/init.d
	chmod +x root/etc/init.d/usb-time-card
	cp usb-time-card.conf root/etc
	cp usb-time-card-deamon root/usr/bin
	cp usb-time-card-deamon.c COPYING README.md root/usr/src/

install: root
	cp -ra root/* /
	update-rc.d usb-time-card defaults

debian: root
	cp -ra DEBIAN root

clean: 
	rm usb-time-card-deamon
	rm root -rf
