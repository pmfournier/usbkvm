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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <inttypes.h>

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

int fd_keyboard = -1;

bool
register_physical_keyboard(struct hid_in *hin)
{
	/* Set up listening for events */

	int fd = open(hin->input_dev, O_RDONLY);
	if (fd == -1) {
		PERROR("open");
		return false;
	}

	int result = ioctl(fd, EVIOCGRAB, 1);
	if (result == -1) {
		PERROR("ioctl(EVIOCGRAB)");
		return false;
	}

	//list_add(kbpt.phys_kb, fd);

	fd_keyboard = fd;

	/* Ok, we have a physical keyboard all setup;
	 * now we can set up an emulated keyboard.
	 */

	//if (!register_emulated_keyboard()) {
	//	ERROR("failed to register emulated keyboard");
	//	return false;
	//}

	return true;
}

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

	if (event_dev == NULL) {
		DEBUG("rejected device because we couldn't find a device for it");
		return false;
	}
	if (usbhid_sys_path == NULL) {
		DEBUG("rejected device because we couldn't find a hid parent for it");
		return false;
	}

	/* USB keyboards have a key bitmap that ends in ffffffff fffffffe */
	const char needle[] = "ffffffff fffffffe";
	if (strlen(capabilities_key) < sizeof(needle) -1) {
		DEBUG("rejected device because it doesn't have the right key capabilities length");
		return false;
	}

	if (strcmp(capabilities_key + strlen(capabilities_key) - (sizeof(needle) - 1), needle) != 0) {
		DEBUG("rejected device because it doesn't have the right key capabilities (%s)", capabilities_key);
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

	if (!register_physical_keyboard(hin)) {
		ERROR("failed to register keyboard");
		return false;
	}

	return true;
}

uint16_t modifiers[] = {
	KEY_LEFTCTRL,
	KEY_LEFTSHIFT,
	KEY_LEFTALT,
	KEY_LEFTMETA,
	KEY_RIGHTCTRL,
	KEY_RIGHTSHIFT,
	KEY_RIGHTALT,
	KEY_RIGHTMETA,
};

int8_t key_to_usb_modifier(uint16_t key)
{
	int i;
	for (i = 0; i < 8; i++) {
		if (key == modifiers[i]) {
			return i;
		}
	}

	return -1;
}

bool input_loop(void)
{
	int result;
	struct input_event ev;
	uint8_t modifier_state = 0;

	int64_t current_key = 0;
	int64_t current_scancode = 0;
	int64_t current_act = -1;

	for (;;) {
		result = read(fd_keyboard, &ev, sizeof(struct input_event));
		if (result == -1) {
			PERROR("read");
			return false;
		}
		if (result == 0) {
			ERROR("Got end of file on evdev");
			return false;
		}

		if (ev.type == EV_SYN) {
			/* Done, check if we have a valid event */
			if (!current_key || !current_scancode || current_act == -1) {
				current_key = 0;
				current_scancode = 0;
				current_act = -1;
				continue;
			}
			ERROR("%s linux key %" PRId64 " (usb %" PRId64 ")", current_act?"Pressed":"Released", current_key, current_scancode);

			char report[8];
			memset(report, 0, sizeof(report));

			int8_t mod = key_to_usb_modifier(current_key);

			if (current_act == 1) {
				if (mod != -1) {
					modifier_state |= (1 << mod);
				} else {
					report[2] = current_scancode;
				}
				report[0] = modifier_state;
				emukb_send_report(report, 8);
			} else {
				if (mod != -1) {
					modifier_state &= ~(1 << mod);
				}
				report[0] = modifier_state;
				emukb_send_report(report, 8);
			}

			current_key = 0;
			current_scancode = 0;
			current_act = -1;
		}

		if (ev.type == EV_KEY && (ev.value == 0 || ev.value == 1)) {
			current_key = ev.code;
			current_act = ev.value;
		}

		if (ev.type == EV_MSC && ev.code == MSC_SCAN) {
			current_scancode = ev.value & 0xffff;
		}

		//DEBUG("Got event %" PRIu16 " %" PRIu16 " %" PRId32, ev.type, ev.code, ev.value);
	}
}

bool run(void) {
	emukb_unregister();
	emukb_register();

	/* Look at installed hardware */
	udev_initial_probe(probe_device);

	input_loop();
	
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
