#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

/* Global variable for SIGTERM handling */
int cont;

/* Determinate, if the USB disk is present */
int USBDiskPresent(const char * serial) {
	/* Lots of things copied from this tutorial:
	   http://www.signal11.us/oss/udev/
	   
	   It says:
	   
	   This code is meant to be a teaching
	   resource. It can be used for anyone for
	   any reason, including embedding into
	   a commercial product. */
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
	/* Create the udev object */
	if (!(udev = udev_new())) {
		perror("Can't create udev");
		exit(1);
	}
	
	/* Create a list of the devices in the 'block' subsystem */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "block");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	
	/* For each item enumerated, chceck if it's our device */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		/* The device pointed to by dev contains information about
		   the block device. In order to get information about the
		   USB device, get the parent device with the
		   subsystem/devtype pair of "usb"/"usb_device". */
		dev = udev_device_get_parent_with_subsystem_devtype(dev,"usb","usb_device");
		if (dev) {
			if (! strcmp(udev_device_get_sysattr_value(dev, "serial"),serial)) {
				/* Free the device and enumerator objects */
				udev_device_unref(dev);
				udev_enumerate_unref(enumerate);
				udev_unref(udev);
				return 1;
			}
			/* Free the device object anyway */
			udev_device_unref(dev);
		}
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
	return 0;       
}

/* Write the status to log */
void writeStatus(const char * serial, int present) {
	/* TODO write to the log */
	if (present) printf("Device %s connected.\n",serial);
	else         printf("Device %s disconnected.\n",serial);
	/* TODO regenerate HTML */
}

/* This happens when SIGTERM */
void term(int signum) {
	/* Do not continue */
	cont = 0;
}


int main(void) {
	int present, waittime;
	char * serial;
	
	/* Handle SIGTERM */
	signal(SIGTERM, term);
	
	/* Continue */
	cont = 1;
	
	/* Device is not present at start */
	present = 0;
	
	/* Default config */
	/* TODO Load config file */
	waittime = 2;
	serial = "7FA11D00715C0087";
	
	/* Main loop */
	while (cont) {
		if (USBDiskPresent(serial) != present) {
			present = ! present;
			writeStatus(serial,present);
		}
		sleep(waittime);
	}
	
	/* Here we are after SIGTERM.
	   If the device is present, fake disconnect */
	if (present) writeStatus(serial,0);
	return 0;
}
