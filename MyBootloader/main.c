
#define F_CPU 16000000UL

#include "uart.h"
#include "i2c.h"
#include "adxl345.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

//-------------------CONSTANTS-----------------
#define VOLTAGE_MIN_ADC		620 //3.0V minimum (3/5 * 1023 = 614, using 620 as safe threshold)
#define TEMP_MIN			0	// 0 deg C min
#define TEMP_MAX			85	// 85 deg C max
#define BLINK_FAST			100 // 100ms fast blink
#define BLINK_SLOW			500 // 500ms slow blink
// addresses
#define EE_BASE_X   0
#define EE_BASE_Y   2
#define EE_BASE_Z   4


//---------------LED HELPER------------------------
static void led_blink(uint8_t count, uint16_t delay_ms){
	for(uint8_t i=0; i<count*2; i++){
		PORTB ^= (1<<5);
		if(delay_ms == BLINK_FAST)  _delay_ms(100);
		else						_delay_ms(500);
	}
	_delay_ms(300); //pause between blink groups
}

static void led_error_halt(uint8_t blink_count, const char* msg){
	uart_print_string("\r\n ERROR: ");
	uart_print_string(msg);
	uart_print_string("\r\n");
	uart_print_string("HALTED\r\n");
	
	while(1){
		led_blink(blink_count, BLINK_FAST);
		_delay_ms(1000); // 1 sec btwn repeats
	}
}

//-----------------------Sensor checks---------------------

//--- check 1: ADXL345 ---
static void check_adxl(int16_t *base_x, int16_t *base_y, int16_t *base_z){
	uart_print_string("Checking ADXL345.......  ");
	I2C_Init();
	ADXL345_Init();
	
	//Read device ID - must be E5
	uint8_t devid = ADXL345_ReadRegister(ADXL_DEVID);
	
	if(devid!= 0xE5){
		led_error_halt(2, "ADXL NOT FOUND");
	}
	
	// Store Baseline XYZ (door closed position)
	ADXL345_ReadXYZ(base_x, base_y, base_z);
	
	uart_print_string("Baseline for ADXL Stored \r\n");
}

//--- Check 2: Internal temp ---
static void check_int_temp(void){
	uart_print_string("checking temperature... ");
	
	// Setup ADC for int temperature sensor
	ADMUX  = (1 << REFS1) | (1 << REFS0) | (1 << MUX3); // Internal 1.1V ref, temp sensor channel
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, prescaler 128
	
	// Discard first reading (ADC needs settling time)
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));

	// Take actual reading
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	uint16_t raw = ADC;
	
	// ATMega328p internal temp sensor formula (from datasheet)
	// T(C) = (raw - 324.32) / 1.22 (approx)
	int16_t temp = (int16_t) ((raw-324)/1.22);
	
	if(temp < TEMP_MIN || temp > TEMP_MAX) {
		led_error_halt(3, "TEMPERATURE OUT OF RANGE");
	}
	
	uart_print_string("OKAY \r\n");
	
	//disable ADC before voltage check reconfigures it
	ADCSRA &= ~(1<<ADEN);
}

//--- Check 3: Voltage (3.3V rail on A0) ---
static void check_voltage(void){
	uart_print_string("Checking Voltage... ");
	
	//SETUP ADC
	ADMUX  = (1 << REFS0);  // AVcc reference (5V), channel 0 (A0)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
	
	//Discard first reading 
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1<< ADSC));
	
	
	// Take actual reading
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	uint16_t raw = ADC;

	//Disable ADC to save power
	ADCSRA &= ~(1<<ADEN);
	
	if(raw < VOLTAGE_MIN_ADC){
		led_error_halt(4, "VOLTAGE TOO LOW");
	}
	
	uart_print_string("OK\r\n");
}

//---Check 4: External Temp ---
static void check_ext_temp(void){
	uart_print_string("Checking external temp...");
	
	//ADC Setup
	ADMUX = (1 << REFS0) | (1 << MUX0);// AVcc reference (5V), channel 1 (A1)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
	
	//Discard first reading
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1<< ADSC));
	
	
	// Take actual reading
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	uint16_t raw = ADC;
	
	// Conversion to temperature value in deg C
	uint16_t voltage = (raw / 1023.0) * 5000;  // millivolts
	uint16_t temp  = voltage / 10.0 ;         // LM35 = 10mV per degree
	
	if(temp < TEMP_MIN || temp > TEMP_MAX) {
		led_error_halt(5, "TEMPERATURE OUT OF RANGE");
	}
	
	uart_print_string("OKAY \r\n");
	
	//disable ADC before voltage check reconfigures it
	ADCSRA &= ~(1<<ADEN);
	
}

static void save_baseline(int16_t x, int16_t y, int16_t z){
	eeprom_write_word((uint16_t*)EE_BASE_X, x);
	eeprom_write_word((uint16_t*)EE_BASE_Y, y);
	eeprom_write_word((uint16_t*)EE_BASE_Z, z);
}

static void jump_to_app(void){
	uint16_t app_word = pgm_read_word(0x0000);
	
	if(app_word == 0xFFFF){
		uart_print_string("NO APP FOUND\r\n");
		while(1){
			led_blink(3, BLINK_SLOW);
		}
	}
	
	uart_print_string("ALL OK... JUMPING TO MAIN APP\r\n");
	_delay_ms(10); //Let uart finish
	
	wdt_enable(WDTO_15MS);
	while(1);
}

//------------- MAIN------------

int main(void) __attribute__((OS_main));

int main(void){
	MCUSR = 0;
	wdt_disable();
	cli();
	
	//LED init
	DDRB |= (1 << 5);
	
	//uart init
	uart_init();
	
    uart_print_string("\r\n========================\r\n");
    uart_print_string("   BOOTLOADER READY\r\n");
    uart_print_string("========================\r\n");
	
	led_blink(3, BLINK_FAST);
	
	//run sensor checks
	uart_print_string("\r\n Running sensor checks\r\n");
	
	int16_t base_x, base_y, base_z;
	
	check_adxl(&base_x, &base_y, &base_z);
	save_baseline(base_x, base_y, base_z);
	check_int_temp();
	check_ext_temp();
	check_voltage();
	
	// ALL checks passed 
	uart_print_string("\r\nALL CHECKS PASSED\r\n");
	led_blink(1, BLINK_SLOW);
	
	jump_to_app();
	
	return 0;
}