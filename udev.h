#ifndef UDEV_H
#define UDEV_H

typedef bool udev_probe_device_f_t(struct udev_device *);

bool udev_initial_probe(udev_probe_device_f_t);

#endif /* UDEV_H */
