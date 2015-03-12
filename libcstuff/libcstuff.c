#include <unistd.h>

#include "libcstuff.h"

int __loglevel = 0;

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

