LDFLAGS=-ludev -g
CFLAGS=-g -Ilibcstuff -Wall -Werror

all: kb

%.o:%.c
	@echo [CC] $<
	@gcc $(CFLAGS) -c $<

libcstuff.o: libcstuff/libcstuff.c
	@gcc $(CFLAGS) -o $@ -c $<

kb: libcstuff.o main.o udev.o
	@echo [LD] $@
	@gcc $(CFLAGS) $(LDFLAGS) -o kb $^

clean:
	rm -f kb
	rm -f *.o

.PHONY: clean
