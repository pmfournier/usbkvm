#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <termios.h>

#include "libcstuff.h"

static size_t emukb_injected_count = 0;

extern const char _binary_report_descriptor_kb_bin_start[];
extern int _binary_report_descriptor_kb_bin_size;
const intptr_t _binary_report_descriptor_kb_bin_size_v =
	(intptr_t) &_binary_report_descriptor_kb_bin_size; 

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

	//"echo musb-hdrc.0.auto >/config/usb_gadget/kb/UDC",
};

static const char *remove_cmds[] = {
	"rm /config/usb_gadget/kb/configs/c.1/hid.usb0",
	"rmdir /config/usb_gadget/kb/functions/hid.usb0",
};

static int emukb_fd = -1;



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
emukb_register_post_enable(void)
{
	const char *gadget_device = "/dev/hidg0";
	int fd = open(gadget_device, O_RDWR);
	if (fd == -1) {
		PERROR("failed to open() %s", gadget_device);
		return false;
	}

	emukb_fd = fd;

	return true;
}

bool
emukb_register_pre_enable(void)
{
	int i;
	for (i = 0; i < array_len(cmds); i++) {
		if (cmds[i] == NULL) {
			if (!write_string_to_file(
				"/config/usb_gadget/kb/functions/hid.usb0/report_desc",
				_binary_report_descriptor_kb_bin_start,
				_binary_report_descriptor_kb_bin_size_v))
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

	return true;
}

int auxfd = -1;

bool emukb_register_auxiliary(const char *filename, int speed)
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

static bool emukb_send_report_aux(void *report, size_t len)
{
	int result;

	char *reportc = (char *)report;

	char fancy_report[3 * len + 1];
	sprintf(fancy_report, "K %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx\n",
		reportc[0],
		reportc[1],
		reportc[2],
		reportc[3],
		reportc[4],
		reportc[5],
		reportc[6],
		reportc[7]);

	DEBUG("Sending report %s", fancy_report);

	result = write_complete(auxfd, fancy_report, strlen(fancy_report));
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

static bool emukb_send_report_native(void *report, size_t len)
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

bool emukb_use_aux = false;

bool emukb_send_report(void *report, size_t len)
{
	if (emukb_use_aux) {
		return emukb_send_report_aux(report, len);
	} else {
		return emukb_send_report_native(report, len);
	}
}

char usb2ascii_array[] = {
	  0,   0,   0,   0, 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
	'3', '4', '5', '6', '7', '8', '9', '0',
	'\n', 27, '\b', '\t', ' ', '-', '=', '[',
	']', '\\', 0, ';', '\'', '`', ',', '.',
	'/',   0,  0,   0,   0,   0,   0,   0,
};

char usb2ascii_shifted_array[] = {
	  0,   0,   0,   0, 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2',
	'3', '4', '5', '6', '7', '8', '9', '0',
	'\n', 27, '\b', '\t', ' ', '_', '+', '{',
	'}', '|', 0, ':', '"', '~', '<', '>',
	0,   0,  0,   0,   0,   0,   0,   0,
};

bool ascii2usb(char c, uint8_t *modifiers_out, char *co)
{
	int i;

	*modifiers_out = 0;

	for (i = 0; i < 64; i++) {
		if (usb2ascii_array[i] == c) {
			*co = i;
			return true;
		}
	}

	for (i = 0; i < 64; i++) {
		if (usb2ascii_shifted_array[i] == c) {
			*modifiers_out = 2; // left shift
			*co = i;
			return true;
		}
	}

	return true;
}

bool usb2ascii(uint8_t c, char *co)
{
	*co = usb2ascii_array[(int)c];

	return true;
}

bool emukb_inject_notrack(const char *text)
{
	char report[8];
	const char *p = text;
	for (;;) {
		if (*p == 0) {
			break;
		}

		memset(report, 0, sizeof(report));

		uint8_t modifiers;
		ascii2usb(*p, &modifiers, &report[2]);
		report[0] = modifiers;

		if (!emukb_send_report(report, 8)) {
			ERROR("failed to send report");
			return false;
		}

		memset(report, 0, sizeof(report));
		if (!emukb_send_report(report, 8)) {
			ERROR("failed to send report");
			return false;
		}

		p++;
	}

	return true;
}

bool emukb_inject(const char *text)
{
	emukb_injected_count += strlen(text);
	return emukb_inject_notrack(text);
}

bool emukb_erase_chars(size_t n_chars)
{
	int i;

	for (i = 0; i < n_chars + 1; i++) {
		emukb_inject("\b");
	}

	return true;
}

bool emukb_erase_injected(void)
{
	int i;

	for (i = 0; i < emukb_injected_count; i++) {
		emukb_inject_notrack("\b");
	}

	emukb_injected_count = 0;

	return true;
}
