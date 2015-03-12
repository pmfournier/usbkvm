#ifndef EMUMOUSE_H
#define EMUMOUSE_H

#include <stdbool.h>

bool
emumouse_register_pre_enable(void);

bool
emumouse_register_post_enable(void);

bool emumouse_register_auxiliary(const char *filename, int speed);

bool
emumouse_unregister(void);

bool emumouse_send_report(void *report, size_t len);

extern bool emumouse_use_aux;

#endif /* EMUMOUSE_H */
