#include <libudev.h>
#include <stdbool.h>
#include <stdio.h>
#include "libcstuff.h"
#include "udev.h"

//bool
//udev_monitor_run(void)
//{
//	/* This section will run continuously, calling usleep() at
//	   the end of each pass. This is to demonstrate how to use
//	   a udev_monitor in a non-blocking way. */
//	while (1) {
//		/* Set up the call to select(). In this case, select() will
//		   only operate on a single file descriptor, the one
//		   associated with our udev_monitor. Note that the timeval
//		   object is set to 0, which will cause select() to not
//		   block. */
//		fd_set fds;
//		struct timeval tv;
//		int ret;
//		
//		FD_ZERO(&fds);
//		FD_SET(fd, &fds);
//		tv.tv_sec = 0;
//		tv.tv_usec = 0;
//		
//		ret = select(fd+1, &fds, NULL, NULL, &tv);
//		
//		/* Check if our file descriptor has received data. */
//		if (ret > 0 && FD_ISSET(fd, &fds)) {
//			printf("\nselect() says there should be data\n");
//			
//			/* Make the call to receive the device.
//			   select() ensured that this will not block. */
//			dev = udev_monitor_receive_device(mon);
//			if (dev) {
//				printf("Got Device\n");
//				printf("   Node: %s\n", udev_device_get_devnode(dev));
//				printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
//				printf("   Devtype: %s\n", udev_device_get_devtype(dev));
//
//				printf("   Action: %s\n",udev_device_get_action(dev));
//				printf("  syspath: %s\n",udev_device_get_syspath(dev));
//				printf("  devpath: %s\n",udev_device_get_devpath(dev));
//				printf("  devnode: %s\n",udev_device_get_devnode(dev));
//				probe_device(dev);
//				udev_device_unref(dev);
//			}
//			else {
//				printf("No Device from receive_device(). An error occured.\n");
//			}					
//		}
//		usleep(250*1000);
//		printf(".");
//		fflush(stdout);
//	}
//
//	return 0;
//}


//bool udev_init_monitor(void)
//{
//	struct udev *udev;
//	struct udev_device *dev;
//	struct udev_monitor *mon;
//	int fd;
//
//	/* Create the udev object */
//	udev = udev_new();
//	if (!udev) {
//		printf("Can't create udev\n");
//		exit(1);
//	}
//	
//
//	/* Set up a monitor to monitor hidraw devices */
//	mon = udev_monitor_new_from_netlink(udev, "udev");
//	udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
//	udev_monitor_enable_receiving(mon);
//	/* Get the file descriptor (fd) for the monitor.
//	   This fd will get passed to select() */
//	fd = udev_monitor_get_fd(mon);
//
//	return true;
//}

bool udev_initial_probe(udev_probe_device_f_t probe_device)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		ERROR("can't create udev");
		return false;
	}
	
	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item enumerated, print out its information.
	   udev_list_entry_foreach is a macro which expands to
	   a loop. The loop will be executed for each member in
	   devices, setting dev_list_entry to a list entry
	   which contains the device's path in /sys. */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		/* usb_device_get_devnode() returns the path to the device node
		   itself in /dev. */
		DEBUG("Device Node Path: %s", udev_device_get_devnode(dev));

		/* From here, we can call get_sysattr_value() for each file
		   in the device's /sys entry. The strings passed into these
		   functions (idProduct, idVendor, serial, etc.) correspond
		   directly to the files in the directory which represents
		   the USB device. Note that USB strings are Unicode, UCS2
		   encoded, but the strings returned from
		   udev_device_get_sysattr_value() are UTF-8 encoded. */
		DEBUG("  VID/PID: %s %s",
		        udev_device_get_sysattr_value(dev,"idVendor"),
		        udev_device_get_sysattr_value(dev, "idProduct"));
		DEBUG("  %s\n  %s",
		        udev_device_get_sysattr_value(dev,"manufacturer"),
		        udev_device_get_sysattr_value(dev,"product"));
		DEBUG("  serial: %s",
		         udev_device_get_sysattr_value(dev, "serial"));

		probe_device(dev);

		udev_device_unref(dev);
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);
	return true;
}
