LDFLAGS=-ludev -g
CFLAGS=-g -Ilibcstuff -Wall -Werror

all: kb

%.o:%.c
	@echo [CC] $<
	@gcc $(CFLAGS) -c $<

report_descriptor.o: report_descriptor.bin
	@echo [BIN] $<
	@ld -r -b binary report_descriptor.bin -o report_descriptor.o

libcstuff.o: libcstuff/libcstuff.c
	@echo [CC] $<
	@gcc $(CFLAGS) -o $@ -c $<

kb: libcstuff.o main.o udev.o emukb.o report_descriptor.o keymap.o
	@echo [LD] $@
	@gcc $(CFLAGS) $(LDFLAGS) -o kb $^

clean:
	rm -f kb
	rm -f *.o

.PHONY: clean
