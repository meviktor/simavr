#undef F_CPU
#define F_CPU 16000000
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega128");

#include "avr/io.h"
#include "avr_lcd.h"

#include <stdbool.h>

#define	__AVR_ATMEGA128__	1
#define LINE_CAPACITY 16

unsigned char data;
unsigned char lineBuff[LINE_CAPACITY];
bool cursorInUpperLine;

// LCD related stuff
void Delay(unsigned int b)
{
  volatile unsigned int a = b;
  while (a)
    a--;
}

void EPulse()
{
	PORTC = PORTC | 0b00000100;	//set E to high
	Delay(1400); 				//delay ~110ms
	PORTC = PORTC & 0b11111011;	//set E to low
}

void LCDInit()
{
	PORTC = PORTC & 0b11111110;
	Delay(10000);

	PORTC = 0b00110000;						//set D4, D5 port to 1
	EPulse();								//high->low to E port (pulse)
	Delay(1000);

	PORTC = 0b00110000;						//set D4, D5 port to 1
	EPulse();								//high->low to E port (pulse)
	Delay(1000);

	PORTC = 0b00110000;						//set D4, D5 port to 1
	EPulse();								//high->low to E port (pulse)
	Delay(1000);

	PORTC = 0b00100000;						//set D4 to 0, D5 port to 1
	EPulse();								//high->low to E port (pulse)

	LCDSendCommand(0x28); 		// function set: 4 bits interface, 2 display lines, 5x8 font
	LCDSendCommand(DISP_OFF); 	// display off, cursor off, blinking off
	LCDSendCommand(CLR_DISP); 	// clear display
	LCDSendCommand(0x06); 		// entry mode set: cursor increments, display does not shift

	LCDSendCommand(DISP_ON);
	LCDSendCommand(CLR_DISP);
}

void LCDSendCommand(unsigned char command)
{
	data = 0b00001111 | command;			//get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;	//set D4-D7
	PORTC = PORTC & 0b11111110;				//set RS port to 0
	EPulse();                              //pulse to set D4-D7 bits

	data = command << 4;					//get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;	//set D4-D7
	PORTC = PORTC & 0b11111110;				//set RS port to 0 -> display set to command mode
	EPulse();                              //pulse to set d4-d7 bits

}

void LCDSendChar(unsigned char character)
{
	data = 0b00001111 | character;		    //get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;	//set D4-D7
	PORTC = PORTC | 0b00000001;				//set RS port to 1
	EPulse();                              //pulse to set D4-D7 bits

	data = character << 4;					//get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;	//clear D4-D7
	PORTC = PORTC | 0b00000001;				//set RS port to 1 -> display set to command mode
	EPulse();                              //pulse to set d4-d7 bits
}

void LCDSendString(char* stirngToSend)
{
	int index;
	for(int index = 0; index < strlen(stirngToSend); index++)
        LCDSendChar(stirngToSend[index]);
}

void PortInit()
{
	PORTA = 0b00011111;		DDRA = 0b01000000; // NOTE: set A4-0 to initialize buttons to unpressed state
	PORTB = 0b00000000;		DDRB = 0b00000000;
	PORTC = 0b00000000;		DDRC = 0b11110111;
	PORTD = 0b11000000;		DDRD = 0b00001000;
	PORTE = 0b00000000;		DDRE = 0b00110000;
	PORTF = 0b00000000;		DDRF = 0b00000000;
	PORTG = 0b00000000;		DDRG = 0b00000000;
}

// Hangman game

int main()
{
	PortInit();
	LCDInit();

	LCDSendString("Hello, world! :) ");
	while(1){}
	
	return 0;
}

