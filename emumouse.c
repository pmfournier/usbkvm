#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <termios.h>

#include "libcstuff.h"

extern const char _binary_report_descriptor_mouse_bin_start[];
extern int _binary_report_descriptor_mouse_bin_size;
const intptr_t _binary_report_descriptor_mouse_bin_size_v =
	(intptr_t) &_binary_report_descriptor_mouse_bin_size; 

//static const char *cmds[] = {
//	"mkdir /config/usb_gadget/mouse",
//	"echo 0x1234 >/config/usb_gadget/mouse/idVendor",
//	"echo 0x5678 >/config/usb_gadget/mouse/idProduct",
//	"echo 0x0100 >/config/usb_gadget/mouse/bcdDevice",
//	"echo 0x0110 >/config/usb_gadget/mouse/bcdUSB",
//	"mkdir /config/usb_gadget/mouse/configs/c.1",
//	// TODO strings
//	"echo 500 >/config/usb_gadget/mouse/configs/c.1/MaxPower",
//
//	"mkdir /config/usb_gadget/mouse/functions/hid.usb0",
//	"echo 1 >/config/usb_gadget/mouse/functions/hid.usb0/subclass", // boot interface subclass
//	"echo 1 >/config/usb_gadget/mouse/functions/hid.usb0/protocol", // keyboard
//	"echo 8 >/config/usb_gadget/mouse/functions/hid.usb0/report_length",
//
//	NULL, // marker to set the record descriptor
//
//	"ln -s /config/usb_gadget/mouse/functions/hid.usb0 /config/usb_gadget/mouse/configs/c.1",
//
//	"echo musb-hdrc.1.auto >/config/usb_gadget/mouse/UDC",
//};

static const char *cmds[] = {

	"mkdir /config/usb_gadget/kb/functions/hid.usb1",
	"echo 1 >/config/usb_gadget/kb/functions/hid.usb1/subclass", // boot interface subclass
	"echo 2 >/config/usb_gadget/kb/functions/hid.usb1/protocol", // mouse
	"echo 8 >/config/usb_gadget/kb/functions/hid.usb1/report_length",

	NULL, // marker to set the record descriptor

	"ln -s /config/usb_gadget/kb/functions/hid.usb1 /config/usb_gadget/kb/configs/c.1",

	//"echo musb-hdrc.0.auto >/config/usb_gadget/kb/UDC",
};

static const char *remove_cmds[] = {
	"rm /config/usb_gadget/kb/configs/c.1/hid.usb1",
	"rmdir /config/usb_gadget/kb/functions/hid.usb1",
};

static int emumouse_fd = -1;


// Find device controllers
//"ls /sys/class/udc"

bool
emumouse_unregister(void)
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
		PERROR("failed to write string to file %s", file);
		return false;
	}
	return true;
}

bool
emumouse_register_pre_enable(void)
{
	int i;
	for (i = 0; i < array_len(cmds); i++) {
		if (cmds[i] == NULL) {
			if (!write_string_to_file(
				"/config/usb_gadget/kb/functions/hid.usb1/report_desc",
				_binary_report_descriptor_mouse_bin_start,
				_binary_report_descriptor_mouse_bin_size_v))
			{
				PERROR("failed to write report descriptor");
				return false;
			}

			continue;
		}

		VERBOSE("running command %s", cmds[i]);
		if (system(cmds[i]) != 0) {
			PERROR("failed to run command [%s]", cmds[i]);
			return false;
		}
	}

	return true;
}

bool
emumouse_register_post_enable(void)
{
	const char *gadget_device = "/dev/hidg1";
	int fd = open(gadget_device, O_RDWR);
	if (fd == -1) {
		PERROR("failed to open() %s", gadget_device);
		return false;
	}

	emumouse_fd = fd;

	return true;
}

static int auxfd = -1;

bool emumouse_register_auxiliary(const char *filename, int speed)
{
	auxfd = open(filename, O_WRONLY);
	if (auxfd == -1) {
		PERROR("open");
		return false;
	}

	struct termios tio;
	if (tcgetattr(auxfd, &tio) == -1) {
		PERROR("tcgetattr");
		return false;
	}

	if (cfsetspeed(&tio, speed) == -1) {
		PERROR("cfsetspeed");
		return false;
	}

	if (tcsetattr(auxfd, TCSANOW, &tio) == -1) {
		PERROR("tcsetattr");
		return false;
	}

	/* Reset the controller's serial link */
	if (write(auxfd, "\n", 1) == -1) {
		PERROR("write");
		return false;
	}

	return true;
}

//static bool emumouse_send_report_aux(void *report, size_t len)
//{
//	int result;
//
//	char *reportc = (char *)report;
//
//	char fancy_report[3 * len + 1];
//	sprintf(fancy_report, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx\n",
//		reportc[0],
//		reportc[1],
//		reportc[2],
//		reportc[3],
//		reportc[4],
//		reportc[5],
//		reportc[6],
//		reportc[7]);
//
//	DEBUG("Sending report %s", fancy_report);
//
//	result = write_complete(auxfd, fancy_report, strlen(fancy_report));
//	if (result == -1) {
//		PERROR("write_complete");
//		return false;
//	}
//	if (result == 0) {
//		ERROR("got end of file from gadget");
//		return false;
//	}
//	return true;
//}

static bool emumouse_send_report_native(void *report, size_t len)
{
	int result;

	//char *reportc = (char *)report;

	//char fancy_report[3 * len + 1];
	//sprintf(fancy_report, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
	//	reportc[0],
	//	reportc[1],
	//	reportc[2],
	//	reportc[3],
	//	reportc[4],
	//	reportc[5],
	//	reportc[6],
	//	reportc[7]);

	//DEBUG("Sending report %s", fancy_report);

	result = write_complete(emumouse_fd, report, len);
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

bool emumouse_use_aux = false;

bool emumouse_send_report(void *report, size_t len)
{
	if (emumouse_use_aux) {
		//return emumouse_send_report_aux(report, len);
	} else {
		return emumouse_send_report_native(report, len);
	}

	return true;
}
