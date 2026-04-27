#ifndef I2C_H_
#define I2C_H_

#include <avr/io.h>

#define F_CPU       16000000UL
#define SCL_CLOCK   100000UL        // 100KHz I2C clock, standard mode

// TWBR formula ? sets I2C clock speed
#define TWBR_VAL    ((F_CPU/SCL_CLOCK) - 16) / 2

// Status codes we care about
#define TW_START         0x08      // START sent
#define TW_REP_START     0x10      // Repeated START sent
#define TW_MT_SLA_ACK    0x18      // Slave address + Write ACK received
#define TW_MT_DATA_ACK   0x28      // Data byte ACK received
#define TW_MR_SLA_ACK    0x40      // Slave address + Read ACK received
#define TW_MR_DATA_ACK   0x50      // Data received ACK sent
#define TW_MR_DATA_NACK  0x58      // Data received NACK sent (last byte)

// Function declarations
void I2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_Write(uint8_t data);
uint8_t I2C_ReadACK(void);         // read byte, send ACK  (more bytes coming)
uint8_t I2C_ReadNACK(void);        // read byte, send NACK (last byte)

#endif /* I2C_H_ */