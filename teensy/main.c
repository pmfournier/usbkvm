#include <stdio.h>
#include <util/delay.h>
#include <avr/io.h>

#include "uart.h"
#include "usb_keyboard.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

void process_buffer(char *buf, size_t buf_len)
{
	int n = sscanf(buf, "%hhx %*x %hhx %hhx %hhx %hhx %hhx %hhx",
		&keyboard_modifier_keys,
		// skipped
		&keyboard_keys[0],
		&keyboard_keys[1],
		&keyboard_keys[2],
		&keyboard_keys[3],
		&keyboard_keys[4],
		&keyboard_keys[5]);

	if (n != 7) {
		uart_putchar('E');
		uart_putchar('1');
		uart_putchar('\n');
		
		return;
	}

	uart_putchar('O');
	uart_putchar('k');
	uart_putchar('\n');

	usb_keyboard_send();
}

int main(void)
{
	CPU_PRESCALE(0);
	uart_init(115200);
	usb_init();

	char buf[3*8];
	size_t buf_len = 0;
	for (;;) {
		buf[buf_len] = uart_getchar();
		if (buf[buf_len] == '\n' || buf[buf_len] == '\r') {
			uart_putchar('S');
			uart_putchar('1');
			uart_putchar('\n');
			process_buffer(buf, buf_len);
			buf_len = 0;
		} else {
			buf_len++;
		}

		if (buf_len == sizeof(buf)) {
			/* Overflow */
			buf_len = 0;
			uart_putchar('E');
			uart_putchar('2');
			uart_putchar('\n');
		}
	}
	return 0;
}
