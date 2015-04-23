/* PMF 2015
 *
 * udev api intro: http://www.signal11.us/oss/udev/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <libudev.h>

#include "udev.h"
#include "libcstuff.h"
#include "emukb.h"

/* A real hid device plugged into this machine's host controller */
struct hid_in {
	/* The event device, typically /dev/input/eventX */
	const char *input_dev;

	const char *usbhid_sys_path;
};

//struct kbpt {
//	list_head phys_kb;
//
//	struct emu_kb emu_kb;
//};
//
//
//bool
//register_physical_keyboard(struct hid_in *hin)
//{
//	/* Set up listening for events */
//
//	int fd = open(hin->input_dev, O_RDONLY);
//	if (fd == -1) {
//		PERROR("open");
//		return false;
//	}
//
//	list_add(kbpt.phys_kb, fd);
//
//	/* Ok, we have a physical keyboard all setup;
//	 * now we can set up an emulated keyboard.
//	 */
//
//	if (!register_emulated_keyboard()) {
//		ERROR("failed to register emulated keyboard");
//		return false;
//	}
//
//	return true;
//}
//
bool
probe_device(struct udev_device *dev)
{
	DEBUG("probe: probing device at %s", udev_device_get_syspath(dev));

	/* 1. Want an input device with an event character device */
	const char *event_dev = NULL;

	/* 2. Want a usb device of driver usbhid */
	const char *usbhid_sys_path = NULL;

	/* 3. capabilities/key */
	const char *capabilities_key = NULL;

	struct udev_device *orig_dev = dev;

	for (; dev != NULL; dev = udev_device_get_parent(dev)) {
		const char *subsys = udev_device_get_subsystem(dev);
		const char *driver = udev_device_get_driver(dev);

		if (!event_dev && subsys && !strcmp(subsys, "input")) {
			event_dev = udev_device_get_devnode(dev);
			/* Could be NULL, in that case we'll try again
			 * at the next opportunity.
			 */
		}

		if (!usbhid_sys_path && subsys && driver && !strcmp(subsys, "usb") && !strcmp(driver, "usbhid")) {
			usbhid_sys_path = udev_device_get_syspath(dev);
			/* Could be NULL, in that case we'll try again
			 * at the next opportunity.
			 */
		}

		if (!capabilities_key) {
			capabilities_key = udev_device_get_sysattr_value(dev, "capabilities/key");
		}

	}

	if (event_dev == NULL || usbhid_sys_path == NULL) {
		return false;
	}

	/* USB keyboards have a key bitmap that ends in fffffffffffffffe */
	const char needle[] = "fffffffffffffffe";
	if (strlen(capabilities_key) < sizeof(needle) -1) {
		return false;
	}

	if (strcmp(capabilities_key + strlen(capabilities_key) - (sizeof(needle) - 1), needle) != 0) {
		return false;
	}

	/* Ok we have a usb device which resolved as hid and input.
	 * Still check if it's a type that interests us.
	 */

	VERBOSE("Got a good device (%s) [path=%s] [%s]", event_dev, udev_device_get_syspath(orig_dev), capabilities_key);

	/* We have a device we're interested in */

	struct hid_in *hin = malloc(sizeof(struct hid_in));
	if (!hin) {
		OOM();
		return false;
	}

	hin->usbhid_sys_path = strdup(usbhid_sys_path);
	if (!hin->usbhid_sys_path) {
		OOM();
		return false;
	}

	hin->input_dev = strdup(event_dev);
	if (!hin->input_dev) {
		OOM();
		return false;
	}

	//if (!register_physical_keyboard(hin)) {
	//	ERROR("failed to register keyboard");
	//	return false;
	//}

	return true;
}

bool run(void) {
	emukb_unregister();
	emukb_register();

	exit(0);

	/* Look at installed hardware */
	udev_initial_probe(probe_device);
	
	return true;
}

bool
parse_opt(int argc, char **argv)
{
	struct option opts[] = {
		{ "verbose", 0, NULL, 1 },
		{ "debug", 0, NULL, 2 },
		{ "help", 0, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};
	
	for (;;) {
		int opt;
		opt = getopt_long(argc, argv, "h", opts, NULL);
		if (opt == -1) {
			/* Finished */
			break;
		}
		
		switch (opt) {
		case 1: /* verbose */
			__loglevel = LOGLEVEL_VERBOSE;
			break;
		case 2: /* debug */
			__loglevel = LOGLEVEL_DEBUG;
			break;
		case 'h':
			//usage(argv[0]);
			exit(0);
		default:
			return false;
		};
	}

	return true;
}

int main(int argc, char **argv)
{
	if (!parse_opt(argc, argv)) {
		ERROR("failed to parse command line options");
		return 1;
	}

	if (!run()) {
		return 1;
	}

	return 0;
}
