#undef F_CPU
#define F_CPU 16000000
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega128");

#include <avr/io.h>
#include "avr_lcd.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#define	__AVR_ATMEGA128__	1

// LCD related stuff
#define LINE_CAPACITY 16

unsigned char data;
unsigned char lineBuff1[LINE_CAPACITY + 1];
//unsigned char lineBuff2[LINE_CAPACITY];
bool cursorAbove;

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

void LCDSendString(char *stirngToSend)
{
	int index;
	for(index = 0; index < strlen(stirngToSend); index++)
        LCDSendChar(stirngToSend[index]);
	while(index < LINE_CAPACITY){
		LCDSendChar(' ');
		index++;
	}
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

// Button related stuff
#define BUTTON_NONE		0
#define BUTTON_CENTER	1
#define BUTTON_LEFT		2
#define BUTTON_RIGHT	3
#define BUTTON_UP		4
#define BUTTON_DOWN		5
bool buttonPressed = false;

int GetPressedButton() {
	// right
	if (!(PINA & 0b00000001) & !buttonPressed) { 
		buttonPressed = true;
		return BUTTON_RIGHT;
	}

	// up
	if (!(PINA & 0b00000010) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_UP;
	}

	// center
	if (!(PINA & 0b00000100) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_CENTER;
	}

	// down
	if (!(PINA & 0b00001000) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_DOWN;
	}

	// left
	if (!(PINA & 0b00010000) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_LEFT;
	}

	return BUTTON_NONE;
}

void TryUnlockButtons() {
	// Check if all buttons are released - all the examined bits are set to 1 in Port A input pin.
	// If so, the sum of these values is 31 = 2^0 + 2^1 + ... + 2^4. No button is pressed.
	if (
		((PINA & 0b00000001)
		|(PINA & 0b00000010)
		|(PINA & 0b00000100)
		|(PINA & 0b00001000)
		|(PINA & 0b00010000)) == 31)
	buttonPressed = false;
}

// Hangman game

// A B C D E F G H I J  K  L  M  N  O | P  Q  R  S  T  U  V  W  X  Y  z
// 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 |15 16 17 18 19 20 21 22 23 24 25
#define LETTERS_FIRST 		"ABCDEFGHIJKLMNO"
#define LETTERS_SECOND 		"PQRSTUVWXYZ"
#define CURSOR_CHAR 		'*'
#define CHOOSEWORD_UI_MIN_ROWINDEX 	0
#define CHOOSEWORD_UI_MAX_ROWINDEX 	2
#define CHOOSEWORD_UI_MIN_COLINDEX 	1
#define CHOOSEWORD_UI_MAX_COLINDEX 	27

#define CHOSEN_WORD_MAX_LENGTH 10
unsigned char chosenWord[CHOSEN_WORD_MAX_LENGTH + 1];
unsigned int letterSelected;

typedef struct {
	int row;
	int col;
} cursor;
cursor cursorPosition;

typedef struct {
	bool updateUI;
	bool wordChosen;
} interaction;

void SetDisplayContent(char *aboveLine, char *belowLine){
	LCDSendCommand(CLR_DISP);
	if(strlen(aboveLine)){
		LCDSendCommand(DD_RAM_ADDR);
		LCDSendString(aboveLine);
	}
	if(strlen(belowLine)){
		LCDSendCommand(DD_RAM_ADDR2);
		LCDSendString(belowLine);
	}
}

void SetLineContent(char *lineContent, bool aboveLine){
	if(strlen(lineContent)){
		LCDSendCommand(aboveLine ? DD_RAM_ADDR : DD_RAM_ADDR2);
		LCDSendString(lineContent);
	}
}

void DrawLetterChooserSection(){
	int index = 0;
	int cursorAdded = 0;
	while(index < LINE_CAPACITY){ // cursor needs a cell position as well
		if(index == (cursorPosition.col % LINE_CAPACITY) && cursorPosition.row == 0){ // first row must be selected
			lineBuff1[index] = CURSOR_CHAR;
			cursorAdded = 1;
		}
		else{
			lineBuff1[index] = 
				// cursor is on the first part of the alphabet or not
				(cursorPosition.col < LINE_CAPACITY) ? LETTERS_FIRST[index - cursorAdded] : ((index - cursorAdded) < strlen(LETTERS_SECOND) ? LETTERS_SECOND[index - cursorAdded] : ' ');
		}
		index = index + 1;
	}
	lineBuff1[index] = '\0';
	// always showing in the line above
	SetLineContent(lineBuff1, true);
}

void DrawChosenWordSection(){
	strcpy(lineBuff1, chosenWord);

	int index = strlen(lineBuff1);
	if(cursorPosition.row == 1){
		lineBuff1[index++] = CURSOR_CHAR;
	}
	while(index < LINE_CAPACITY){
		lineBuff1[index++] = ' ';
	}
	// showing in the row above if we are all the the way down, in the bottom row otherwise
	SetLineContent(lineBuff1, cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX); 
}

void DrawOK(){
	strcpy(lineBuff1, "OK");

	int index = strlen(lineBuff1);
	lineBuff1[index++] = CURSOR_CHAR;
	while(index < LINE_CAPACITY){
		lineBuff1[index++] = ' ';
	}
	// always showing in the bottom row
	SetLineContent(lineBuff1, false);
}

void ReDrawChooseWordUI(){
	// if we are not at the bottom row, show letter chooser
	if(cursorPosition.row < CHOOSEWORD_UI_MAX_ROWINDEX){
		DrawLetterChooserSection();
	}
	// show the word itself
	DrawChosenWordSection();
	// if we are at the bottom line, show OK option to accept the word
	if(cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX){
		DrawOK();
	}
}

interaction HandleChooseWordBtnPress(){
	interaction userInteraction;

	int pressedBtn = BUTTON_NONE;
	while((pressedBtn = GetPressedButton()) == BUTTON_NONE)
		TryUnlockButtons();

	switch(pressedBtn){
		case BUTTON_RIGHT:
			cursorPosition.col = cursorPosition.col == CHOOSEWORD_UI_MAX_COLINDEX ? 
				cursorPosition.col : 
				(cursorPosition.col + 1) == LINE_CAPACITY ? LINE_CAPACITY + 1 : (cursorPosition.col + 1);
			break;
		case BUTTON_LEFT:
			cursorPosition.col = cursorPosition.col == CHOOSEWORD_UI_MIN_COLINDEX ? 
				cursorPosition.col : 
				(cursorPosition.col - 1) == LINE_CAPACITY ? LINE_CAPACITY - 1 : (cursorPosition.col - 1);
			break;
		case BUTTON_UP:
			cursorPosition.row = cursorPosition.row == CHOOSEWORD_UI_MIN_ROWINDEX ? cursorPosition.row : cursorPosition.row - 1;
			break;
		case BUTTON_DOWN:
			cursorPosition.row = cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX ? cursorPosition.row : cursorPosition.row + 1;
			break;
	}

	if(pressedBtn != BUTTON_CENTER){
		userInteraction.updateUI = true;
		userInteraction.wordChosen = false;
	}
	else{
		// standing on OK
		if(cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX){
			userInteraction.updateUI = false;
			userInteraction.wordChosen = true;
		}
		// standing on row of the chosen word or letter chooser
		else{
			userInteraction.updateUI = true;
			userInteraction.wordChosen = false;
			// standing on letter chooser
			if(cursorPosition.row == CHOOSEWORD_UI_MIN_ROWINDEX){
				if(letterSelected < CHOSEN_WORD_MAX_LENGTH){
					// the cursor points always the letter on the left of the cursor (having index lower by one)
					chosenWord[letterSelected] = 
						(cursorPosition.col < LINE_CAPACITY) ? LETTERS_FIRST[cursorPosition.col - 1] : LETTERS_SECOND[cursorPosition.col % LINE_CAPACITY - 1];
					letterSelected = letterSelected + 1;
				}
			}
			// standing on the row of the chosen word
			else{
				// deleting last letter
				letterSelected = letterSelected > 0 ? letterSelected - 1 : 0;
				chosenWord[letterSelected] = '\0';	
			}
		}
	}

	return userInteraction;
}

void ChooseWord(){
	cursorPosition.col = 1;
	cursorPosition.row = 0;
	letterSelected = 0;
	chosenWord[letterSelected] = '\0';

	ReDrawChooseWordUI();

	interaction userInteraction = {.updateUI = false, .wordChosen = false};
	while(!userInteraction.wordChosen){
		userInteraction = HandleChooseWordBtnPress();
		if(userInteraction.updateUI){
			ReDrawChooseWordUI();
		}
	}
	chosenWord[letterSelected] = '\0';
}

int main()
{
	PortInit();
	LCDInit();
	
	SetDisplayContent("Hangman game", "by Viktor");
	// press center button to start
	while (GetPressedButton() != BUTTON_CENTER)
			TryUnlockButtons();

	while(1){
		ChooseWord();
		SetDisplayContent("Word chosen is:", chosenWord);
		while (GetPressedButton() != BUTTON_CENTER)
			TryUnlockButtons();
	}
	
	return 0;
}

