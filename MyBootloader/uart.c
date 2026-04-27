#include "uart.h"

void uart_init(void){
	// set baud rate
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char)MYUBRR;
	// enable transmitter and receiver
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	// Set frame format: 8data, 1 stop bit, no parity
	UCSR0C = (3<<UCSZ00);
}

void uart_transmit(uint8_t data){
	// wait for empty transmit buffer
	while(!(UCSR0A & (1<<UDRE0)));
	// send the data
	UDR0 = data;
	
}

void uart_print_string(const char* str){
	if (!str) return;
	while(*str!='\0'){
		uart_transmit(*str++);
	}
}

uint8_t uart_receive_timeout(uint16_t timeout_ms, uint8_t *received){
	// Each iteration is approximately 1ms at 16MHz
	uint32_t counts = (uint32_t)timeout_ms * 1600;
	
	while(counts--){
		if(UCSR0A & (1 << RXC0)){
			*received = UDR0;
			return 1;  // Success - data received
		}
	}
	return 0;  // Timeout - no data came
}