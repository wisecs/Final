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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "acx.h"

#define SENSOR_PIN 0
#define SENSOR_MASK 1 << SENSOR_PIN
#define LED_PIN 1
#define LED_MASK 1 << LED_PIN

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
void temp_to_string(char *, word);
float temp_to_f(word);
void update_lamp(float);
//char * serial_read_word(void);


float TLOW = 80;     // temp low
float THIGH = 80.5;  // temp high
int PERIOD = 10;      // time in seconds between output
bool PRINT = true;   // whether to receive output or not
bool HEATING = false;// heater state
int rx_len = 0;		//length of the last received word
volatile bool RX_FLAG = false;	//true if a word has been received
volatile char rx_word[100];		//last serial word received
volatile int rx_i = 0;			//index for receiving a serial word


int main(void) {
	_delay_ms(1000);
	serial_setup();
	sei();
	
	while(true) {

		if(RX_FLAG == true) {
			cli();
			char * word_copy;
			strcpy(word_copy, rx_word);

			serial_write_word(word_copy);
			RX_FLAG = false;

			const char delim[2] = "\r";
			word_copy = strtok(word_copy, delim);

			for(int i = 0; i < rx_len; i++) {
				word_copy[i] = tolower(word_copy[i]);
			}

			if(!strncmp(word_copy, "set", 3)) {
				if(!strcmp(word_copy, "set")) {
					//returns the values of all the parameter settings (TLOW, THIGH, RATE, HEX, PERIOD)
				} else if(!strcmp(word_copy, "set on")) {
					//set on
				} else if(!strcmp(word_copy, "set off")) {
					//set off					
				} else if(strchr(word_copy, '=') != NULL) { //Split
					const char delim2[2] = "=";
					char * argument;
					word_copy = strtok(word_copy, delim2);
					argument = strtok(NULL, delim2);
					
					if(!strcmp(word_copy, "set hex")) {
						if(!strcmp(argument, "on")) {
							//set hex=on
						} else if(!strcmp(argument, "off")) {
							//set hex=off
						} else {
							//error
						}
					} else if(!strcmp(word_copy, "set tlow")) {
						//check that argument is a number
						//set tlow
					} else if(!strcmp(word_copy, "set thigh")) {
						//check that argument is a number
						//set thigh
					} else if(!strcmp(word_copy, "set period")) {
						//check that argument is a number
						//set thigh
					} else {
						//error
					}
				} else {
					//error
				}
			} else if(!strncmp(word_copy, "help", 4)) {
				//help
			} else {
				
			}
			
			sei();
		}
	}

	/*int timer = 0;
	while (1) {
		word temp = sensor_read();
		float tempf = temp_to_f(temp);
		update_lamp(tempf);
		
		char tempWord[6];
		temp_to_string(tempWord, temp);
		
		if(PRINT && timer == PERIOD) {
			serial_write_word(tempWord);
			timer = 0;
		}
		
		timer++;
		_delay_ms(1000);
	}*/
}

void update_lamp(float temp) {
	DDRF |= LED_MASK;
	
	if(temp < TLOW) {
		PORTF |= LED_MASK;
		HEATING = true;
		} else if(temp > THIGH) {
		PORTF &= ~LED_MASK;
		HEATING = false;
	}
}

float temp_to_f(word temp) {
	bool negative = 0x8000 & temp;
	temp &= ~0x8000;
	
	float fah = (float) temp / 10.0;
	fah = fah * 1.8 + 32;
	if(negative)
	fah *= -1;
	
	return fah;
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

void temp_to_string(char * tempArray, word temp) {
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
	//UDR0 = rx_word[rx_i];	//Echo back

	if(rx_word[rx_i] == 0x0A || rx_word[rx_i] == 0x0D || rx_word[rx_i] == '\0') {
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