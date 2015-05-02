#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdbool.h>
#include <stdint.h>

bool keymap_init(void);
bool keymap_map(uint8_t from, uint8_t *to);
bool keymap_remap(uint8_t from, uint8_t to);

#endif /* KEYMAP_H */
