#include "libcstuff.h"

static const char *cmds[] = {
	"mkdir /config/usb_gadget/kb",
	"cd /config/usb_gadget/kb",
	"echo 0x1234 >idVendor",
	"echo 0x5678 >idProduct",
	"echo 0x0100 >bcdDevice",
	"echo 0x0110 >bcdUSB",
	"mkdir configs/c.1",
	// TODO strings
	"echo 500 >configs/c.1/MaxPower",

	"mkdir functions/hid.usb0",
	"echo 1 >functions/hid.usb0/subclass", // boot interface subclass
	"echo 1 >functions/hid.usb0/protocol", // keyboard

	"ln -s functions/hid.usb0 configs/c.1",

	"echo musb-hdrc.0.auto >UDC",
};


// Find device controllers
"ls /sys/class/udc"

bool
emukb_register(void)
{
	int i;
	for (i = 0; i < array_len(cmds); i++) {
		VERBOSE("running command %s", cmds[i]);
		if (system(cmds[i] != 0)) {
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
