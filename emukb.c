#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "libcstuff.h"

static const char *cmds[] = {
	"mkdir /config/usb_gadget/kb",
	"echo 0x1234 >/config/usb_gadget/kb/idVendor",
	"echo 0x5678 >/config/usb_gadget/kb/idProduct",
	"echo 0x0100 >/config/usb_gadget/kb/bcdDevice",
	"echo 0x0110 >/config/usb_gadget/kb/bcdUSB",
	"mkdir /config/usb_gadget/kb/configs/c.1",
	// TODO strings
	"echo 500 >/config/usb_gadget/kb/configs/c.1/MaxPower",

	"mkdir /config/usb_gadget/kb/functions/hid.usb0",
	"echo 1 >/config/usb_gadget/kb/functions/hid.usb0/subclass", // boot interface subclass
	"echo 1 >/config/usb_gadget/kb/functions/hid.usb0/protocol", // keyboard

	"ln -s /config/usb_gadget/kb/functions/hid.usb0 /config/usb_gadget/kb/configs/c.1",

	"echo musb-hdrc.0.auto >/config/usb_gadget/kb/UDC",
};

static const char *remove_cmds[] = {
	"rm /config/usb_gadget/kb/configs/c.1/hid.usb0",
	"rmdir /config/usb_gadget/kb/configs/c.1",
	"rmdir /config/usb_gadget/kb/functions/hid.usb0",
	"rmdir /config/usb_gadget/kb",
};


// Find device controllers
//"ls /sys/class/udc"

bool
emukb_unregister(void)
{
	/* Don't care about errors here */
	int i;
	for (i = 0; i < array_len(remove_cmds); i++) {
		DEBUG("running command %s", remove_cmds[i]);
		if (system(remove_cmds[i]) != 0) {
			DEBUG("failed to run command [%s] but ignoring", remove_cmds[i]);
		}
	}

	return true;
}

bool
emukb_register(void)
{
	int i;
	for (i = 0; i < array_len(cmds); i++) {
		VERBOSE("running command %s", cmds[i]);
		if (system(cmds[i]) != 0) {
			ERROR("failed to run command [%s]", cmds[i]);
			return false;
		}
	}

	const char *gadget_device = "/dev/hidg0";
	int fd = open(gadget_device, O_RDWR);
	if (fd == -1) {
		PERROR("failed to open() %s", gadget_device);
		return false;
	}

	/* FIXME: add a way to read and write to the device */

	return true;
}
