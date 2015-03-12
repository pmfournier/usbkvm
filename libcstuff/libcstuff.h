#ifndef LIBCSTUFF_H
#define LIBCSTUFF_H

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define LOGLEVEL_VERBOSE 1
#define LOGLEVEL_DEBUG 2

extern int __loglevel;

#define LOG(min_level, __msg, __args...) \
	do { \
		if (__loglevel >= min_level) { \
			fprintf(stderr,  __msg " (" __FILE__ ":" STRINGIFY(__LINE__) ")" "\n", ##__args); \
		} \
	} while (0)
#define ERROR(__msg, __args...) LOG(0, "error: " __msg, ##__args)
#define PERROR(__msg, __args...) ERROR(__msg " (%s)", ##__args, strerror(errno))
#define VERBOSE(__msg, __args...) LOG(1, __msg, ##__args)
#define DEBUG(__msg, __args...) LOG(2, __msg, ##__args)
#define OOM() ERROR("OUT OF MEMORY")

#define array_len(__arr) (sizeof(__arr) / sizeof(__arr[0]))

int write_complete(int fd, const void *buf, size_t len);

#endif /* LIBCSTUFF_H */
