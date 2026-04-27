#define F_CPU 16000000UL

#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"
#include "i2c.h"
#include "adxl345.h"

// EEPROM Address
#define EE_BASE_X   0
#define EE_BASE_Y   2
#define EE_BASE_Z   4
// ADXL threshold
#define ADXL_THRESHOLD 20
#define TEMP_MAX 85

// Voltage thresholds (ADC raw)
#define HALL_CLOSED  614  // 3.0V
#define HALL_OPEN     61  // 0.3V

static uint16_t check_voltage(void){	
	//SETUP ADC
	ADMUX  = (1 << REFS0);  // AVcc reference (5V), channel 0 (A0)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
	
	// Take reading
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	uint16_t raw = ADC;
	
	return raw;
}

static uint16_t check_temp(void){	
	//ADC Setup
	ADMUX = (1 << REFS0) | (1 << MUX0);// AVcc reference (5V), channel 1 (A1)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
	
	// Take reading
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	uint16_t raw = ADC;
	
	// Conversion to temperature value in deg C
	uint16_t voltage = (raw / 1023.0) * 5000;  // millivolts
	int16_t temp  = voltage / 10.0 ;         // LM35 = 10mV per degree
	
	return temp;

}

int main(void)
{
    int16_t base_x = eeprom_read_word((uint16_t*)EE_BASE_X);
	int16_t base_y = eeprom_read_word((uint16_t*)EE_BASE_Y);
	int16_t base_z = eeprom_read_word((uint16_t*)EE_BASE_Z);
	int16_t cur_x, cur_y, cur_z;
	DDRB |= (1 << 5);
	uart_init();
	I2C_Init();
	ADXL345_Init();
	
    while (1) 
    {
		uint8_t door_open_checkpoint = 0;	
		//    - Read live ADXL XYZ
		ADXL345_ReadXYZ(&cur_x, &cur_y, &cur_z);
		//    - Compare to baseline ±20
		if(cur_x > base_x + 20 || cur_x < base_x - 20 || cur_y > base_y + 20 || cur_y < base_y - 20 || cur_z > base_z + 20 || cur_z < base_z - 20){
			door_open_checkpoint++;
		}
		//    - Read voltage on A0
		uint16_t hall_sensor_value = check_voltage();
		//DOOR OPEN
		if (hall_sensor_value < HALL_OPEN) door_open_checkpoint++;		
		//    - Read LM35 on A1
		int16_t temp_val = check_temp();
		if(temp_val>TEMP_MAX){
			uart_print_string("Temperature too high, exiting code...");
			break;
		}
		//    - Determine door state
		if(door_open_checkpoint == 2){
			uart_print_string("DOOR OPEN");
		}
		else {
			uart_print_string("DOOR CLOSED");
		}
		 _delay_ms(500);
    }
}

