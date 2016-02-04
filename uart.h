#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>

void uart_init(void);
void uart_putc(uint8_t c);
uint8_t uart_getc(void);
uint16_t uart_bytes_available(void);

#endif /* _UART_H_ */
