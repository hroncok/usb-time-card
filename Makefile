CC?=gcc
CFLAGS=-Wall -pedantic -g
LIBS=-ludev -lconfig
VERSION=$(shell grep "define VERSION" usb-time-card-deamon.c | cut -f3 -d" " | cut -f2 -d\")

ifeq "$(shell uname -m)" "x86_64"
	ARCH?=amd64
else
	ARCH?=i386
endif

ifeq "$(ARCH)" "i386"
	M=-m32
else
	M=-m64
endif


all: usb-time-card-deamon

usb-time-card-deamon: usb-time-card-deamon.c
	$(CC) $(M) -o $@ $^ $(CFLAGS) $(LIBS)

root: usb-time-card-deamon usb-time-card usb-time-card.conf usb-time-card.1
	rm root -rf
	mkdir -p root/usr/bin
	mkdir -p root/usr/src
	mkdir -p root/etc/init.d
	mkdir -p root/usr/share/man/man1
	cp usb-time-card root/etc/init.d
	chmod +x root/etc/init.d/usb-time-card
	cp usb-time-card.conf root/etc
	cp usb-time-card-deamon root/usr/bin
	cp usb-time-card.1 root/usr/share/man/man1
	gzip root/usr/share/man/man1/usb-time-card.1
	cp usb-time-card-deamon.c COPYING README.md root/usr/src/

install: root
	cp -ra root/* /
	update-rc.d usb-time-card defaults

debian: root
	rm root/DEBIAN -rf
	cp -ra DEBIAN root
	chmod -R 755 root/DEBIAN
	sed -i "s/VERSION/$(VERSION)/" root/DEBIAN/control
	sed -i "s/ARCH/$(ARCH)/" root/DEBIAN/control
	sed -i "s/SIZE/$$((`du -s root | cut -f1`-`du -s root/DEBIAN | cut -f1`))/" root/DEBIAN/control
	dpkg -b root usb-time-card_$(VERSION)_$(ARCH).deb

clean: 
	rm usb-time-card-deamon root *.deb -rf
