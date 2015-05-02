#include "keymap.h"

static uint8_t keymap[256];

bool keymap_init(void)
{
	int i;
	for (i = 0; i < 256; i++) {
		keymap[i] = i;
	}
	return true;
}

bool keymap_map(uint8_t from, uint8_t *to)
{
	*to = keymap[from];
	return true;
}

bool keymap_remap(uint8_t from, uint8_t to)
{
	keymap[from] = to;
	return true;
}
