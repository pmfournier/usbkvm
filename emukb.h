#ifndef EMUKB_H
#define EMUKB_H

#include <stdbool.h>

bool
emukb_register_pre_enable(void);

bool
emukb_register_post_enable(void);

bool emukb_register_auxiliary(const char *filename, int speed);

bool
emukb_unregister(void);

bool emukb_inject(const char *text);

bool emukb_send_report(void *report, size_t len);

bool usb2ascii(char c, char *co);

bool emukb_erase_injected(void);

bool emukb_erase_chars(size_t n_chars);

extern bool emukb_use_aux;

#endif /* EMUKB_H */
