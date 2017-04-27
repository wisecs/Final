/**
 * Final.c
 *
 * Created: 4/20
 * Author : Carter W, Drake S
 * 
 * 5V pin for sensor power
 * PORTF pin 0 (A0 on board) for sensor data IN
 * PORTF pin 1 (A1 on board) for LED
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>
#include <acx.h>

#define SENSOR_PIN = 0
#define SENSOR_MASK = 1 << SENSOR_PIN
#define LED_PIN = 1
#define LED_MASK = 1 << LED_PIN

typedef uint8_t byte;
typedef uint16_t word;

void delay_usec(byte);
void serial_setup(void);
void serial_write(char);
void serial_write_word(char *);
char serial_read(void);
//char * serial_read_word(void);

int main(void) {
   serial_setup();
   sensor_setup();
    while (1) 
    {
       
    }
}

word sensor_read() {
   //Send signal to tell sensor to send data
   PORTF &= !SENSOR_MASK;
   _delay_ms(5);
   PORTF |= SENSOR_MASK;
   delay(40);
   
   //Check if low
   delay(80);
   //Check if high
   delay(80);
   
   //Humidity
   word temp = 0;
   for(int i = 0; i < 16; i++) {
      //start of number
      delay(50);
      delay(30);
      if((PINF & SENSOR_MASK == 0))
         temp &= ~0x01;
      else {
         temp |= 0x01;
         delay(40);
      }         
      temp = temp << 1;
   }
   
   //Temperature
   word humid = 0;
   for(int i = 0; i < 16; i++) {
      //start of number
      delay(50);
      delay(30);
      if((PINF & SENSOR_MASK == 0))
      humid &= ~0x01;
      else {
         humid |= 0x01;
         delay(40);
      }
      humid = humid << 1;
   }
   
   //Checksum
   byte check = 0;
   for(int i = 0; i < 8; i++) {
      //start of number
      delay(50);
      delay(30);
      if((PINF & SENSOR_MASK == 0))
      check &= ~0x01;
      else {
         check |= 0x01;
         delay(40);
      }
      check = check << 1;
   }
   
}

void delay(int us) {;
   int curr_pin = PINF & SENSOR_MASK;
   
   int loop_end = us / 5;
   for(int i = 0; i < loop_end; i++) {
      if((PINF & SENSOR_MASK) != curr_pin)
         break;
     delay_usec(5);
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