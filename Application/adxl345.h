#ifndef ADXL345_H_
#define ADXL345_H_

#include <avr/io.h>
#include "i2c.h"

// I2C Address (SDO connected to GND)
#define ADXL_ADDR           0x53

// I2C read/write address bytes
// I2C requires address shifted left + R/W bit
#define ADXL_WRITE          (ADXL_ADDR << 1)        // 0xA6
#define ADXL_READ           (ADXL_ADDR << 1 | 0x01) // 0xA7

// Registers
#define ADXL_DEVID          0x00
#define ADXL_POWER_CTL      0x2D
#define ADXL_DATA_FORMAT    0x31
#define ADXL_DATAX0         0x32
#define ADXL_DATAX1         0x33
#define ADXL_DATAY0         0x34
#define ADXL_DATAY1         0x35
#define ADXL_DATAZ0         0x36
#define ADXL_DATAZ1         0x37

// Function declarations
void ADXL345_Init(void);
uint8_t ADXL345_ReadRegister(uint8_t reg);
void ADXL345_WriteRegister(uint8_t reg, uint8_t value);
void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);

#endif /* ADXL345_H_ */