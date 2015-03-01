#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

#include "libcstuff.h"

extern const char _binary_report_descriptor_bin_start[];
extern int _binary_report_descriptor_bin_size;
const intptr_t _binary_report_descriptor_bin_size_v =
	(intptr_t) &_binary_report_descriptor_bin_size; 

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
	"echo 8 >/config/usb_gadget/kb/functions/hid.usb0/report_length",

	NULL, // marker to set the record descriptor

	"ln -s /config/usb_gadget/kb/functions/hid.usb0 /config/usb_gadget/kb/configs/c.1",

	"echo musb-hdrc.0.auto >/config/usb_gadget/kb/UDC",
};

static const char *remove_cmds[] = {
	"rm /config/usb_gadget/kb/configs/c.1/hid.usb0",
	"rmdir /config/usb_gadget/kb/configs/c.1",
	"rmdir /config/usb_gadget/kb/functions/hid.usb0",
	"rmdir /config/usb_gadget/kb",
};

static int emukb_fd = -1;


int write_complete(int fd, const void *buf, size_t len)
{
	size_t orig_len = len;

	for (;;) {
		int result = write(fd, buf, len);
		if (result == -1) {
			return -1;
		}
		if (result == 0) {
			return 0;
		}
		len -= result;
		buf = ((char *)buf) + result;


		if (len == 0) {
			return orig_len;
		}
	}
}


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

static bool write_string_to_file(const char *file, const char *str, size_t len)
{
	int fd = open(file, O_WRONLY | O_TRUNC);
	if (fd == -1) {
		PERROR("open");
		return false;
	}

	if (write_complete(fd, str, len) == -1) {
		ERROR("failed to write string to file %s", file);
		return false;
	}
	return true;
}

bool
emukb_register(void)
{
	int i;
	for (i = 0; i < array_len(cmds); i++) {
		if (cmds[i] == NULL) {
			if (!write_string_to_file(
				"/config/usb_gadget/kb/functions/hid.usb0/report_desc",
				_binary_report_descriptor_bin_start,
				_binary_report_descriptor_bin_size_v))
			{
				PERROR("failed to write report descriptor");
				return false;
			}

			continue;
		}

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

	emukb_fd = fd;

	return true;
}

bool emukb_send_report(void *report, size_t len)
{
	int result;

	char *reportc = (char *)report;

	char fancy_report[3 * len + 1];
	sprintf(fancy_report, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
		reportc[0],
		reportc[1],
		reportc[2],
		reportc[3],
		reportc[4],
		reportc[5],
		reportc[6],
		reportc[7]);

	DEBUG("Sending report %s", fancy_report);

	result = write_complete(emukb_fd, report, len);
	if (result == -1) {
		PERROR("write_complete");
		return false;
	}
	if (result == 0) {
		ERROR("got end of file from gadget");
		return false;
	}
	return true;
}
