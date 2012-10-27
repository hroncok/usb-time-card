#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <libconfig.h>
#include <time.h>

/* Global variable for SIGTERM handling */
int gcont;

/* Default configuration */
void defaultConfig(int * waittime,const char ** serial,const char ** log,const char ** html) {
	*waittime = 300;
	*serial = "111111111111111";
	*log = "/var/log/usb-time-card.log";
	*html = "/var/www/usb-time-card/index.html";
}

/* In file configuration */
void loadConfig(config_t *cf, const char * config, int * waittime,const char ** serial,const char ** log,const char ** html) {
	/* Default config */
	defaultConfig(waittime,serial,log,html);
	
	config_init(cf);
	
	/* Check config file syntax */
	if (!config_read_file(cf, config)) {
		fprintf(stderr, "%s:%d - %s\n",config_error_file(cf),config_error_line(cf),config_error_text(cf));
		config_destroy(cf);
		exit(1);
	}
	
	/* Load variables */
	config_lookup_int(cf, "waittime", waittime);
	config_lookup_string(cf, "serial", serial);
	config_lookup_string(cf, "log", log);
	config_lookup_string(cf, "html", html);
	
	/* Validation */
	if (*waittime < 0) {
		fprintf(stderr, "Found negative value in waittime variable in config file. Not going to work.\n");
		config_destroy(cf);
		exit(1);
	}
	
	/* Destroy config before exiting the program */
}

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

/* Read the log and rebuild HTML page */
void exportHTML(const char * log, const char * html) {
	FILE *logfile;
	FILE *htmlfile;
	
	/* Open logfile for reading */
	logfile = fopen(log,"r");
	if (!logfile) {
		perror("Cannot open log file for reading");
		exit(1);
	}
	
	/* Open HTML file for (re)writing */
	htmlfile = fopen(html,"w");
	if (!htmlfile) {
		perror("Cannot open HTML file for writing");
		exit(1);
	}
	
	/* HTML head */
	fprintf(htmlfile,"<html>\n");
	fprintf(htmlfile,"<head>\n");
	fprintf(htmlfile,"\t<title>USB Time Card Log</title>\n");
	fprintf(htmlfile,"\t<meta charset=\"utf-8\">\n");
	fprintf(htmlfile,"</head>\n");
	fprintf(htmlfile,"<body>\n");
	
	/* TODO Stuff here */
	
	/* HTML tail */
	fprintf(htmlfile,"</body>\n");
	fprintf(htmlfile,"</html>\n");
	
	/* Close files */
	fclose(logfile);
	fclose(htmlfile);
}

/* Write the status to log */
void writeStatus(const char * serial, int present, const char * log, const char * html) {
	const char *way;
	time_t now;
	FILE *logfile;
	
	/* In or out */
	if (present) way = " in";
	else         way = "out";
	time(&now);
	
	/* Open logfile for appending */
	logfile = fopen(log,"a");
	if (!logfile) {
		perror("Cannot open log file for writing");
		exit(1);
	}
	fprintf(logfile,"%s: %s %s",serial,way,ctime(&now));
	fclose(logfile);
	
	exportHTML(log,html);
}

/* This happens when SIGTERM */
void term(int signum) {
	/* Do not continue */
	gcont = 0;
}

int main(void) {
	int present, waittime;
	const char *config, *serial, *log, *html;
	config_t cfg;
	
	/* Config file */
	/* TODO load from --config option */
	config = "/home/churchyard/tmp/skola/ADS/usb-time-card/usb-time-card.conf";
	
	/* Handle SIGTERM and more */
	signal(SIGTERM, term);
	signal(SIGHUP, term);
	signal(SIGINT, term);
	
	/* Continue */
	gcont = 1;
	
	/* Device is not present at start */
	present = 0;
	
	/* Load config from file */
	loadConfig(&cfg,config,&waittime,&serial,&log,&html);
	
	/* Main loop */
	while (gcont) {
		if (USBDiskPresent(serial) != present) {
			present = ! present;
			writeStatus(serial,present,log,html);
		}
		sleep(waittime);
	}
	
	/* Here we are after SIGTERM.
	   If the device is present, fake disconnect */
	if (present) writeStatus(serial,0,log,html);
	
	/* Destroy config */
	config_destroy(&cfg);
	return 0;
}
