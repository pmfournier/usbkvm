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

bool emumouse_send_report(uint8_t but, int16_t x, int16_t y, int8_t wheel);

extern bool emumouse_use_aux;

#endif /* EMUMOUSE_H */
