/**
* Final.c
*
* Created: 4/20
* Author : Carter W, Drake S
*
* 5V pin for sensor power
* PORTF pin 0 (A0 on board) for sensor data IN
* PORTF pin 1 (A1 on board) for LED
* http://yaab-arduino.blogspot.com/2015/12/how-to-sprintf-float-with-arduino.html
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "acx.h"

#define SENSOR_PIN 0
#define SENSOR_MASK 1 << SENSOR_PIN
#define LED_PIN 1
#define LED_MASK 1 << LED_PIN
#define HELP_MESSAGE "HELP - displays a message showing commands and brief descriptions\rSET  - returns the values of all the parameter settings (TLOW, THIGH, HEX, PERIOD)\rSET ON  - enables controller to send readings to host\rSET OFF - disables controller from sending readings to host\rSET HEX=<ON or OFF> - switches reading format between ASCII and HEX (see below)\rSET TLOW=<value>   - low temperature setting. If temp is below this, LED/heater should be turned on.\rSET THIGH=<value> - high temperature setting. If temp is above this, LED/heater should be turned off.\rSET PERIOD=<value>  - time in seconds between readings sent to the host. An integer value not less than 2, not greater than 65535.\r"

typedef uint8_t byte;
typedef uint16_t word;

void delay_usec(byte);
void polled_wait(void);
void serial_setup(void);
void serial_write(char);
void serial_write_word(char *);
char serial_read(void);
void sensor_setup(void);
word sensor_read(void);
void sensor_to_string(char *, word);
void ftemp_to_string(char *, float);
float sensor_to_F(word);
void update_lamp(float);

void terminal_thread();
void sensor_thread();


volatile float TLOW = 80;     // temp low
volatile float THIGH = 80.5;  // temp high
volatile int PERIOD = 3;      // time in seconds between output
volatile bool PRINT = true;   // whether to receive output or not
volatile bool HEX = false;	//whether to print in hex
volatile bool HEATING = false;// heater state
volatile int rx_len = 0;		//length of the last received word
volatile bool RX_FLAG = false;	//true if a word has been received
volatile char rx_word[100];		//last serial word received
volatile int rx_i = 0;			//index for receiving a serial word


int main(void) {
	_delay_ms(1000);
	serial_setup();
	x_init();
	x_new(1, sensor_thread, true);
	x_new(2, terminal_thread, true);
	sei();

	while(true) {
		x_yield();
	}
}

void terminal_thread(void) {
	while(true) {
		if(RX_FLAG == true) {
			cli();
			char word_arr[100];
			char * word_copy = word_arr;
			strcpy(word_copy, rx_word);

			serial_write_word(word_copy);
			serial_write('\r');
			RX_FLAG = false;

			for(int i = 0; i < rx_len; i++) {
				word_copy[i] = tolower(word_copy[i]);
			}

			if(!strncmp(word_copy, "set", 3)) {
				if(!strcmp(word_copy, "set")) {
					serial_write_word("TLOW = ");
					char tmp_temp[5];
					ftemp_to_string(tmp_temp, TLOW);
					serial_write_word(tmp_temp);
					serial_write_word("F\rTHIGH = ");
					ftemp_to_string(tmp_temp, THIGH);
					serial_write_word(tmp_temp);
					
					if(HEX)
					serial_write_word("F\rHEX ON\r");
					else
					serial_write_word("F\rHEX OFF\r");

					char set_print[25];
					sprintf(set_print, "PERIOD = %d seconds\r", PERIOD);
					serial_write_word(set_print);
					} else if(!strcmp(word_copy, "set on")) {
					PRINT = true;
					} else if(!strcmp(word_copy, "set off")) {
					PRINT = false;
					} else if(strchr(word_copy, '=') != NULL) { //Split
					const char delim2[2] = "=";
					char * argument;
					word_copy = strtok(word_copy, delim2);
					argument = strtok(NULL, delim2);
					
					if(!strcmp(word_copy, "set hex")) {
						if(!strcmp(argument, "on"))
						HEX = true;
						else if(!strcmp(argument, "off"))
						HEX = false;
						else
						serial_write_word("Incorrect command\r");
						} else if(!strcmp(word_copy, "set tlow")) {
						float low = atof(argument);
						if(low != 0) //atof worked
						TLOW = low;
						else
						serial_write_word("Incorrect temperature value\r");
						} else if(!strcmp(word_copy, "set thigh")) {
						float high = atof(argument);
						if(high != 0) //atof worked
						THIGH = high;
						else
						serial_write_word("Incorrect temperature value\r");
						} else if(!strcmp(word_copy, "set period")) {
						int time = atoi(argument);
						if(!(time < 2)) //either too low or overflowed high
						PERIOD = time;
						else
						serial_write_word("Incorrect period value\r");
					} else
					serial_write_word("Incorrect command\r");
				} else
				serial_write_word("Incorrect command\r");
			} else if(!strncmp(word_copy, "help", 4))
			serial_write_word(HELP_MESSAGE);
			else
			serial_write_word("Incorrect command\r");
			sei();
		}
		x_delay(100);
	}
}

void sensor_thread(void) {
	DDRF |= LED_MASK;
	int timer = 0;

	while (1) {
		word temp = sensor_read();
		float tempf = sensor_to_F(temp);
		update_lamp(tempf);
	
		if(PRINT && timer == PERIOD) {
			if(HEX) {
				char tempWord[30];
				unsigned long int time = x_gtime();
				word tmp_temp;
				word low;
				float tmp_low = TLOW;
				word high;
				float tmp_high = THIGH;
				word test;

				void * p1 = &tmp_temp;
				void * p2 = &tempf;
				memcpy(p1, p2, 4);

				p1 = &low;
				p2 = &tmp_low;
				memcpy(p1, p2, 4);

				p1 = &high;
				p2 = &tmp_high;
				memcpy(p1, p2, 4);

				//Variable names don't match up with what they contain
				sprintf(tempWord, "%lx %x %x %d\r", time, tmp_temp, low, HEATING);
				serial_write_word(tempWord);

				/*tmp_temp = ((union { float f; unsigned int i; }){tempf}).i;
				sprintf(tempWord, "%x\r", tmp_temp);
				serial_write_word(tempWord);*/

				/*//Printing out Current Temp
				void * p1 = &tmp_temp;
				void * p2 = &tempf;
				memcpy(p1, p2, 4);

				p1 = &test;
				p2 = &tempf;
				memcpy(p1, p2, 4);
				sprintf(tempWord, "%lx %x ", time, tmp_temp);
				serial_write_word(tempWord);

				//Printing out Low
				p1 = &low;
				p2 = &tmp_low;
				memcpy(p1, p2, 4);
				
				p1 = &test;
				p2 = &tmp_low;
				memcpy(p1, p2, 4);
				sprintf(tempWord, "%x ", low);
				serial_write_word(tempWord);

				//Printing out High
				p1 = &high;
				p2 = &tmp_high;
				memcpy(p1, p2, 4);

				p1 = &test;
				p2 = &tmp_high;
				memcpy(p1, p2, 4);
				sprintf(tempWord, "%x %d\r", high, HEATING);
				serial_write_word(tempWord);*/
			} else {
				char tempWord[15];
				//Timestamp
				unsigned long int time = x_gtime();
				sprintf(tempWord, "%lu: ", time);
				serial_write_word(tempWord);

				//Current Temp
				ftemp_to_string(tempWord, tempf); 
				serial_write_word(tempWord);
				serial_write_word(", ");

				//Low Temp
				ftemp_to_string(tempWord, TLOW);
				serial_write_word(tempWord);
				serial_write_word(", ");

				//High Temp
				ftemp_to_string(tempWord, THIGH);
				serial_write_word(tempWord);
				serial_write_word(", ");

				if(HEATING)
				serial_write_word("HEATER ON\r");
				else
				serial_write_word("HEATER OFF\r");
			}

			timer = 0;
		}
	
		timer++;
		x_delay(1000);
	}
}

void update_lamp(float temp) {
	if(temp < TLOW)
		HEATING = true;
	else if(temp > THIGH) 
		HEATING = false;
	
	if(HEATING)
		PORTF |= LED_MASK;
	else
		PORTF &= ~LED_MASK;
	
}

word sensor_read(void) {
	cli();
	
	//Send signal to tell sensor to send data
	DDRF |= SENSOR_MASK;    //Sets SENSOR_PIN for output from the temperature sensor
	PORTF &= !SENSOR_MASK;  //Push pin LOW
	_delay_ms(5);
	DDRF &= ~SENSOR_MASK;   //Sets SENSOR_PIN for input from the temperature sensor
	PORTF |= SENSOR_MASK;   //Sets SENSOR_PIN to idle at high
	polled_wait();          //Should begin receiving in 20-40 microseconds
	
	//Check if low
	//delay_usec(80);
	polled_wait();
	//Check if high
	polled_wait();
	//elay_usec(85);
	
	word humidity = 0;
	for(int i = 0; i < 16; i++) {
		polled_wait();
		delay_usec(40);
		
		if(PINF & SENSOR_MASK) { //Bit is a 1
			humidity = humidity << 1;
			humidity |= 0x0001;
			delay_usec(40);
			} else {//Bit is a zero
			humidity = humidity << 1;
			//humidity = humidity & ~1;
		}
	}
	
	word temp = 0;
	for(int i = 0; i < 16; i++) {
		polled_wait();
		delay_usec(40);
		
		if(PINF & SENSOR_MASK) { //Bit is a 1
			temp = temp << 1;
			temp |= 0x0001;
			delay_usec(40);
			} else {//Bit is a zero
			temp = temp << 1;
			//humidity = humidity & ~1;
		}
	}
	
	byte checksum = 0;
	for(int i = 0; i < 8; i++) {
		polled_wait();
		delay_usec(40);
		
		if(PINF & SENSOR_MASK) { //Bit is a 1
			checksum = checksum << 1;
			checksum |= 0x01;
			delay_usec(40);
			} else {//Bit is a zero
			checksum = checksum << 1;
		}
	}
	
	sei();
	return temp;
}

void ftemp_to_string(char * tempArray, float temp) {
	temp = temp * 10.0;
	int whole = temp / 10;
	int decimal = (int)temp % 10;
	sprintf(tempArray, "%d.%d", whole, decimal);
}

void sensor_to_string(char * tempArray, word temp) {
	bool negative = 0x8000 & temp;
	temp &= ~0x8000;
	//calculation to F
	temp = temp * 1.8 + 320;
	
	if(negative)
	tempArray[0] = '-';
	else
	tempArray[0] = ' ';
	char * tempPointer = &tempArray[1];
	
	int whole = temp / 10;
	int decimal = temp % 10;
	sprintf(tempPointer, "%d.%d", whole, decimal);
}

float sensor_to_F(word temp) {
	bool negative = 0x8000 & temp;
	temp &= ~0x8000;
	
	float fah = (float) temp / 10.0;
	fah = fah * 1.8 + 32;
	if(negative)
	fah *= -1;
	
	return fah;
}

void polled_wait(void) {
	int curr_pin = PINF & SENSOR_MASK;
	
	while((PINF & SENSOR_MASK) == curr_pin) {
		//delay_usec(5);
	}
}

void sensor_setup(void) {
	DDRF &= ~SENSOR_MASK;   //Sets SENSOR_PIN for input from the temperature sensor
	PORTF |= SENSOR_MASK;   //Sets SENSOR_PIN to idle at high temperature
}

//Sets up USART0 for 8N1, 9600 baud communication
void serial_setup(void) {
	UCSR0A = 0x00;
	UCSR0B = 0x00;
	UCSR0C = 0x00;
	
	UCSR0B |= 1 << RXEN0;   //Enable receive
	UCSR0B |= 1 << TXEN0;   //Enable transmit
	UCSR0B |= 1 << RXCIE0;	//Enable receive interrupt
	
	UCSR0C |= 1 << UCSZ00;  //Setting number of data bits to 8
	UCSR0C |= 1 << UCSZ01;
	
	UBRR0 = 103; //Setting up baud rate for 9600 baud
}

//Writes the parameter to USART0
void serial_write(char data) {
	while(!(UCSR0A & (1 << UDRE0))) {
		//Wait until data register is empty
	}
	UDR0 = data;
}

void serial_write_word(char * data) {
	for(int i = 0; data[i] != '\0'; i++) {
		serial_write(data[i]);
	}
}

//Reads and returns value in USART0
char serial_read(void) {
	while(!(UCSR0A & (1 << RXC0))) {
		//wait until Receive Complete
	}
	return UDR0;
}

ISR(USART0_RX_vect) {
	rx_word[rx_i] = UDR0;

	if(rx_word[rx_i] == 0x0A || rx_word[rx_i] == 0x0D || rx_word[rx_i] == '\0') {
		rx_word[rx_i] = '\0';
		RX_FLAG = true;
		rx_len = rx_i-1;
		rx_i = 0;
	} else 
		rx_i++;
}

/*char * serial_read_word() {
static char data[20];
int i = 0;
uint32_t word_done = 0;
while(true) {
while(!(UCSR0A & (1 << RXC0)) || word_done != UINT32_MAX) {
//wait until Receive Complete
word_done++;
}
if(word_done == UINT32_MAX) {
data[i] = '\0';
break;
}

data[i] = UDR0;
i++;
word_done = 0;
}

return data;

}*/