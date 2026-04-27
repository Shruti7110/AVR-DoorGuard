#ifndef UART_H
#define UART_H

#include <avr/io.h>
#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1)

void uart_init(void);
void uart_transmit(uint8_t data);
void uart_print_string(const char* str);
uint8_t uart_receive_timeout(uint16_t timeout_ms, uint8_t *received);


#endif