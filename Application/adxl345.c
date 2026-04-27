
#include "adxl345.h"

/* Writes a value to a specific ADXL345 register
 * Used to configure sensor settings like power mode, range, etc.
 */
void ADXL345_WriteRegister(uint8_t reg, uint8_t value)
{
	I2C_Start();
	I2C_Write(ADXL_WRITE);     // send address + write bit
	I2C_Write(reg);            // send register address
	I2C_Write(value);          // send value
	I2C_Stop();
}

/* Reads a single register from ADXL345
 * Sends register address over I2C and returns the received value
 */
uint8_t ADXL345_ReadRegister(uint8_t reg)
{
	uint8_t data;

	I2C_Start();
	I2C_Write(ADXL_WRITE);     // send address + write bit (to set register)
	I2C_Write(reg);            // send register we want to read

	I2C_Start();               // repeated START
	I2C_Write(ADXL_READ);      // send address + read bit
	data = I2C_ReadNACK();     // read one byte, send NACK (only 1 byte)
	I2C_Stop();

	return data;
}

/* Initializes ADXL345 accelerometer
 * Sets up SPI communication and configures required registers
 * (e.g., enables measurement mode, sets data format/range)
 */
void ADXL345_Init(void)
{
	// Wake up sensor ? enable measurement mode
	ADXL345_WriteRegister(ADXL_POWER_CTL, 0x08);

	// Full resolution, +/- 2g range
	ADXL345_WriteRegister(ADXL_DATA_FORMAT, 0x08);
}

/* Reads X, Y, Z acceleration data from ADXL345
 * Retrieves 6 bytes (2 bytes per axis) and combines into 16-bit values
 * Stores results in provided pointers (x, y, z)
 */
void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
	uint8_t x0, x1, y0, y1, z0, z1;

	x0 = ADXL345_ReadRegister(ADXL_DATAX0);
	x1 = ADXL345_ReadRegister(ADXL_DATAX1);
	y0 = ADXL345_ReadRegister(ADXL_DATAY0);
	y1 = ADXL345_ReadRegister(ADXL_DATAY1);
	z0 = ADXL345_ReadRegister(ADXL_DATAZ0);
	z1 = ADXL345_ReadRegister(ADXL_DATAZ1);

	*x = (int16_t)((x1 << 8) | x0);
	*y = (int16_t)((y1 << 8) | y0);
	*z = (int16_t)((z1 << 8) | z0);
}