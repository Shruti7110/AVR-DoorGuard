#include "i2c.h"

void I2C_Init(void)
{
	TWSR = 0x00;                // prescaler = 1
	TWBR = (uint8_t)TWBR_VAL;  // set clock speed
	TWCR = (1 << TWEN);        // enable I2C
}

void I2C_Start(void)
{
	// Send START condition
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while(!(TWCR & (1 << TWINT)));   // wait until done
}

void I2C_Stop(void)
{
	// Send STOP condition
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void I2C_Write(uint8_t data)
{
	TWDR = data;                             // load data
	TWCR = (1 << TWINT) | (1 << TWEN);      // start transmission
	while(!(TWCR & (1 << TWINT)));           // wait until done
}

uint8_t I2C_ReadACK(void)
{
	// Read byte and send ACK ? tells sensor more bytes needed
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	while(!(TWCR & (1 << TWINT)));
	return TWDR;
}

uint8_t I2C_ReadNACK(void)
{
	// Read byte and send NACK ? tells sensor this is last byte
	TWCR = (1 << TWINT) | (1 << TWEN);
	while(!(TWCR & (1 << TWINT)));
	return TWDR;
}