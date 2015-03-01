#ifndef EMUKB_H
#define EMUKB_H

#include <stdbool.h>

bool
emukb_register(void);

bool
emukb_unregister(void);

bool emukb_send_report(void *report, size_t len);
#endif /* EMUKB_H */
