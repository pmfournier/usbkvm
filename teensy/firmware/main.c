#include <stdio.h>
#include <util/delay.h>
#include <avr/io.h>

#include "uart.h"
#include "usb_keyboard.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

void process_buffer_kb(char *buf, size_t buf_len)
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
		// Parse error reading keyboard report
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

void process_buffer_mouse(char *buf, size_t buf_len)
{
	int8_t x,y,wheel;

	int n = sscanf(buf, "%hhx %hhx %hhx %hhx",
		&mouse_buttons,
		&x,
		&y,
		&wheel);

	if (n != 4) {
		// Parse error reading mouse report
		uart_putchar('E');
		uart_putchar('5');
		uart_putchar('\n');
		
		return;
	}

	uart_putchar('O');
	uart_putchar('k');
	uart_putchar('\n');

	usb_mouse_send(x,y,wheel);
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

			if (buf_len < 2) {
				// Input line too short
				uart_putchar('E');
				uart_putchar('3');
				uart_putchar('\n');
				buf_len = 0;
			}
			if (buf[0] == 'K') {
				process_buffer_kb(buf + 2, buf_len - 2);
			} else if (buf[0] == 'M') {
				process_buffer_mouse(buf + 2, buf_len - 2);
			} else {
				// Invalid peripheral specification
				uart_putchar('E');
				uart_putchar('4');
				uart_putchar('\n');
				buf_len = 0;
			}
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
