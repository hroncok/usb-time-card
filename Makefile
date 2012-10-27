CC=gcc
CFLAGS=-Wall -pedantic
LIBS=-ludev -lconfig

usb-time-card-deamon: usb-time-card-deamon.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

all: usb-time-card-deamon
