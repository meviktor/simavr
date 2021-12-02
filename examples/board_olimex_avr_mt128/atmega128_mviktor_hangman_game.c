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

void LCDSendString(char *stirngToSend, bool fillSpaces)
{
	int index;
	for(index = 0; index < strlen(stirngToSend); index++)
        LCDSendChar(stirngToSend[index]);
	if(fillSpaces){
		while(index < LINE_CAPACITY){
			LCDSendChar(' ');
			index++;
		}
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
		return BUTTON_UP;
	}

	// up
	if (!(PINA & 0b00000010) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_LEFT;
	}

	// center
	if (!(PINA & 0b00000100) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_CENTER;
	}

	// down
	if (!(PINA & 0b00001000) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_RIGHT;
	}

	// left
	if (!(PINA & 0b00010000) & !buttonPressed) {
		buttonPressed = true;
		return BUTTON_DOWN;
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

#define LINE_CAPACITY_LETTERCHOOSER 14
// A B C D E F G H I J  K  L  M | N  O  P  Q  R  S  T  U  V  W  X  Y  Z
// 0 1 2 3 4 5 6 7 8 9 10 11 12 |13 14 15 16 17 18 19 20 21 22 23 24 25
#define LETTERS_FIRST 		"ABCDEFGHIJKLM"
#define LETTERS_SECOND 		"NOPQRSTUVWXYZ"
#define CURSOR_CHAR 		4
#define UP_ARROW_CHAR 		5
#define DOWN_ARROW_CHAR 	6
#define CHOOSEWORD_UI_MIN_ROWINDEX 	0
#define CHOOSEWORD_UI_MAX_ROWINDEX 	2
#define CHOOSEWORD_UI_MIN_COLINDEX 	1
#define ALPHABET_LETTERS 26
#define CHOOSEWORD_UI_MAX_COLINDEX ALPHABET_LETTERS + 1 // colindex "14" is not used because of paging

#define CHOSEN_WORD_MAX_LENGTH 10
unsigned char chosenWord[CHOSEN_WORD_MAX_LENGTH + 1];
unsigned int letterSelected;

#define MAX_BAD_GUESS_ATTEMPTS 10
#define GAME_NOT_OVER_YET 0
#define GAME_OVER_WON 1
#define GAME_OVER_LOST 2
bool lettersUsed[ALPHABET_LETTERS];
int numberOfGuesses;
unsigned char goodGuesses[CHOSEN_WORD_MAX_LENGTH + 1];
unsigned int gameResult;

typedef struct {
	int row;
	int col;
} cursor;
cursor cursorPosition;

typedef struct {
	bool updateUI;
	bool wordChosen;
	int gameState;
} interaction;

int BadGuesses(){
	return numberOfGuesses - (int)strlen(goodGuesses);
}

void WaitForBtnPress(int buttonCode){
	while (GetPressedButton() != buttonCode)
			TryUnlockButtons();
}

void SetDisplayContent(char *aboveLine, char *belowLine){
	LCDSendCommand(CLR_DISP);
	if(strlen(aboveLine)){
		LCDSendCommand(DD_RAM_ADDR);
		LCDSendString(aboveLine, true);
	}
	if(strlen(belowLine)){
		LCDSendCommand(DD_RAM_ADDR2);
		LCDSendString(belowLine, true);
	}
}

void SetLineContent(char *lineContent, bool aboveLine, bool fillSpaces){
	if(strlen(lineContent)){
		LCDSendCommand(aboveLine ? DD_RAM_ADDR : DD_RAM_ADDR2);
		LCDSendString(lineContent, fillSpaces);
	}
}

void DrawLetterChooserSection(){
	int index = 0;
	int cursorAdded = 0;
	while(index < LINE_CAPACITY_LETTERCHOOSER){ // cursor needs a cell position as well
		if(index == (cursorPosition.col % LINE_CAPACITY_LETTERCHOOSER) && cursorPosition.row == 0){ // first row must be selected
			lineBuff1[index] = CURSOR_CHAR;
			cursorAdded = 1;
		}
		else{
			// cursor is on the first part of the alphabet or not
			lineBuff1[index] = (cursorPosition.col < LINE_CAPACITY_LETTERCHOOSER) ? 
				LETTERS_FIRST[index - cursorAdded] : ((index - cursorAdded) < strlen(LETTERS_SECOND) ? LETTERS_SECOND[index - cursorAdded] : ' ');
		}
		index = index + 1;
	}
	lineBuff1[index] = '\0';
	// always showing in the line above
	SetLineContent(lineBuff1, true, false);
}

unsigned char CUSTOM_POINTER_ARROW[8] = { 0b00000, 0b00100, 0b01000, 0b11111, 0b01000, 0b00100, 0b00000, 0b00000 };
unsigned char CUSTOM_UP_ARROW[8] = { 0b00000, 0b00100, 0b01110, 0b10101, 0b00100, 0b00100, 0b00100, 0b00000 };
unsigned char CUSTOM_DOWN_ARROW[8] = { 0b00000, 0b00100, 0b00100, 0b00100, 0b10101, 0b01110, 0b00100, 0b00000 };

void InitCustomArrows(){
	LCDSendCommand(0x40 + 32); // CGRAM
	// characters with code 4, 5, 6
	int j;
	for(j = 0; j < 8; j++)
		LCDSendChar(CUSTOM_POINTER_ARROW[j]);
	for(j = 0; j < 8; j++)
		LCDSendChar(CUSTOM_UP_ARROW[j]);
	for(j = 0; j < 8; j++)
		LCDSendChar(CUSTOM_DOWN_ARROW[j]);
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
	SetLineContent(lineBuff1, cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX, true);

	LCDSendCommand(cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX ? DD_RAM_ADDR + 15 : DD_RAM_ADDR2 + 15);
	LCDSendChar(cursorPosition.row == CHOOSEWORD_UI_MAX_ROWINDEX ? UP_ARROW_CHAR : DOWN_ARROW_CHAR);

	// The last two cells of the letter chooser section is not modified, so unused arrows stays there sometimes
	if(cursorPosition.row != CHOOSEWORD_UI_MAX_ROWINDEX){
		LCDSendCommand(DD_RAM_ADDR + 15);
		LCDSendChar(' ');
	}
}

void DrawOK(){
	strcpy(lineBuff1, "OK");

	int index = strlen(lineBuff1);
	lineBuff1[index++] = CURSOR_CHAR;
	while(index < LINE_CAPACITY){
		lineBuff1[index++] = ' ';
	}
	// always showing in the bottom row
	SetLineContent(lineBuff1, false, true);
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
				(cursorPosition.col + 1) == LINE_CAPACITY_LETTERCHOOSER ? LINE_CAPACITY_LETTERCHOOSER + 1 : (cursorPosition.col + 1);
			break;
		case BUTTON_LEFT:
			cursorPosition.col = cursorPosition.col == CHOOSEWORD_UI_MIN_COLINDEX ? 
				cursorPosition.col : 
				(cursorPosition.col - 1) == LINE_CAPACITY_LETTERCHOOSER ? LINE_CAPACITY_LETTERCHOOSER - 1 : (cursorPosition.col - 1);
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
			// empty words are not accpeted
			userInteraction.wordChosen = strlen(chosenWord) > 0 ? true : false;
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
						(cursorPosition.col < LINE_CAPACITY_LETTERCHOOSER) ? LETTERS_FIRST[cursorPosition.col - 1] : LETTERS_SECOND[(cursorPosition.col % LINE_CAPACITY_LETTERCHOOSER) - 1];
					letterSelected = letterSelected + 1;
				}
			}
			// standing on the row of the chosen word
			else{
				// deleting last letter
				letterSelected = letterSelected > 0 ? letterSelected - 1 : 0;	
			}
			chosenWord[letterSelected] = '\0';
		}
	}

	return userInteraction;
}

void ChooseWord(){
	cursorPosition.col = 1;
	cursorPosition.row = 0;
	letterSelected = 0;
	chosenWord[letterSelected] = '\0';

	InitCustomArrows();
	ReDrawChooseWordUI();

	interaction userInteraction = {.updateUI = false, .wordChosen = false};
	while(!userInteraction.wordChosen){
		userInteraction = HandleChooseWordBtnPress();
		if(userInteraction.updateUI){
			ReDrawChooseWordUI();
		}
	}
}

unsigned char BELOW_RIGHT_STATES[56] = { // 7 * 8
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, // 0
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11110, 0b00000, // 1
	0b01000, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11110, 0b00000, // 6
	0b11000, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11110, 0b00000, // 7
	0b11110, 0b01001, 0b00000, 0b00000, 0b00000, 0b00000, 0b11110, 0b00000, // 8
	0b11110, 0b01001, 0b10000, 0b10000, 0b00000, 0b00000, 0b11110, 0b00000, // 9
	0b11110, 0b01001, 0b10100, 0b10100, 0b00010, 0b00000, 0b11110, 0b00000  //10
};

unsigned char BELOW_LEFT_STATES[40] = { // 5 * 8
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, // 0
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01111, 0b00000, // 1
	0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01111, 0b00000, // 2
	0b01001, 0b01010, 0b01000, 0b01000, 0b01000, 0b01000, 0b01111, 0b00000, // 7
	0b01001, 0b01010, 0b01000, 0b01000, 0b01001, 0b01000, 0b01111, 0b00000  // 9
};

unsigned char ABOVE_LEFT_STATES[32] = { // 4 * 8
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, // 0
	0b00000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, // 2
	0b00000, 0b01000, 0b01010, 0b01100, 0b01000, 0b01000, 0b01000, 0b01000, // 3
	0b00000, 0b01111, 0b01010, 0b01100, 0b01000, 0b01000, 0b01000, 0b01000  // 4
};

unsigned char ABOVE_RIGHT_STATES[32] = { // 4 * 8
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, // 0
	0b00000, 0b11110, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, // 4
	0b00000, 0b11110, 0b01000, 0b01000, 0b11100, 0b10100, 0b11100, 0b00000, // 5
	0b00000, 0b11110, 0b01000, 0b01000, 0b11100, 0b10100, 0b11100, 0b01000  // 6
};

// Describes which version should we use of the four picture-parts when we already have 0, 1, ..., 10 bad guesses.
// Values are the starting indexes of the currently used picture-part versions in their arrays:
// BELOW_RIGHT_STATES, BELOW_LEFT_STATES, ABOVE_LEFT_STATES, ABOVE_RIGHT_STATES
unsigned char STATE_COMBINATIONS[11][4] = {
	{ 0,  0,  0,  0},
	{ 8,  8,  0,  0},
	{ 8, 16,  8,  0},
	{ 8, 16, 16,  0},
	{ 8, 16, 24,  8},
	{ 8, 16, 24, 16},
	{16, 16, 24, 24},
	{24, 24, 24, 24},
	{32, 24, 24, 24},
	{40, 32, 24, 24},
	{48, 32, 24, 24}
};

void HangmanAreaInit(){
	LCDSendCommand(DD_RAM_ADDR + 14); // top line, penult cell
	LCDSendChar(2);
	LCDSendChar(3);
	LCDSendCommand(DD_RAM_ADDR2 + 14); // bottom line, penult cell
	LCDSendChar(1);
	LCDSendChar(0);
}

void SetHangmanState(int badGuesses){
	LCDSendCommand(0x40); // CGRAM
	// characters with code 0, 1, 2, 3
	int j;
	for(j = STATE_COMBINATIONS[badGuesses][0]; j < STATE_COMBINATIONS[badGuesses][0] + 8; j++)
		LCDSendChar(BELOW_RIGHT_STATES[j]);
	for(j = STATE_COMBINATIONS[badGuesses][1]; j < STATE_COMBINATIONS[badGuesses][1] + 8; j++)
		LCDSendChar(BELOW_LEFT_STATES[j]);
	for(j = STATE_COMBINATIONS[badGuesses][2]; j < STATE_COMBINATIONS[badGuesses][2] + 8; j++)
		LCDSendChar(ABOVE_LEFT_STATES[j]);
	for(j = STATE_COMBINATIONS[badGuesses][3]; j < STATE_COMBINATIONS[badGuesses][3] + 8; j++)
		LCDSendChar(ABOVE_RIGHT_STATES[j]);
}

/**
 * Returns the player has found the word or not.
 */
bool WordHasBeenGuessed(){
	int index;
	for(index = 0; index < strlen(chosenWord); index++){
		if(strchr(goodGuesses, chosenWord[index]) == NULL)
			return false;
	}
	return true;
}

void DrawWordToGuessSection(){
	LCDSendCommand(DD_RAM_ADDR2);
	int index;
	for(index = 0; index < strlen(chosenWord); index++)
		LCDSendChar(strchr(goodGuesses, chosenWord[index]) != NULL ? chosenWord[index] : '_');
}

void ReDrawGuessWordUI(){
	DrawLetterChooserSection();
	DrawWordToGuessSection();
	SetHangmanState(BadGuesses());
}

/**
 * Returns the first index for the cursor through which it can point on a letter which has not been selected yet.
 * @param searchRight: searching wheter the right or the left direction.
 */
int findUnSelectedLetterIndex(bool searchRight){
	int searchIndex = searchRight ? 
		// When "paging" the letter list, we want to ensure that the cursor stays on the right side of the selected character
		((cursorPosition.col + 1) == LINE_CAPACITY_LETTERCHOOSER ? LINE_CAPACITY_LETTERCHOOSER + 1 : (cursorPosition.col + 1)) :
		((cursorPosition.col - 1) == LINE_CAPACITY_LETTERCHOOSER ? LINE_CAPACITY_LETTERCHOOSER - 1 : (cursorPosition.col - 1));
	if(searchRight){
		while(searchIndex <= CHOOSEWORD_UI_MAX_COLINDEX){
			// need the minus one cause searchIndex still a column index - need to resolve "letter index"
			if((searchIndex < LINE_CAPACITY_LETTERCHOOSER && !lettersUsed[searchIndex - 1]) ||
				// using minus two because from the second page colposition 15 points to the letter N - having index 13 in "lettersUsed" array, and so on...
			   (searchIndex > LINE_CAPACITY_LETTERCHOOSER && !lettersUsed[searchIndex - 2])){
				return searchIndex;
			}
			searchIndex = (searchIndex + 1) == LINE_CAPACITY_LETTERCHOOSER ? LINE_CAPACITY_LETTERCHOOSER + 1 : searchIndex + 1;
		}
	}
	else{
		while(searchIndex >= CHOOSEWORD_UI_MIN_COLINDEX){
			if((searchIndex < LINE_CAPACITY_LETTERCHOOSER && !lettersUsed[searchIndex - 1]) ||
				// using minus two because: see above
			   (searchIndex > LINE_CAPACITY_LETTERCHOOSER && !lettersUsed[searchIndex - 2])){
				return searchIndex;
			}
			searchIndex = (searchIndex - 1) == LINE_CAPACITY_LETTERCHOOSER ? LINE_CAPACITY_LETTERCHOOSER - 1 : searchIndex - 1;
		}
	}
	return cursorPosition.col;
}

interaction HandleGuessWordBtnPress(){
	interaction userInteraction = {.updateUI = false, .gameState = GAME_NOT_OVER_YET };
	int pressedBtn = BUTTON_NONE;

	while((pressedBtn = GetPressedButton()) == BUTTON_NONE)
		TryUnlockButtons();

	if(pressedBtn == BUTTON_LEFT || pressedBtn == BUTTON_RIGHT || pressedBtn == BUTTON_CENTER){
		unsigned char chosenChar;
		switch(pressedBtn){
			case BUTTON_RIGHT:
				// when selecting a letter which was already selected, we have to shift further the cursor to the right to selet a letter which has not been selected yet
				cursorPosition.col = findUnSelectedLetterIndex(true);
				break;
			case BUTTON_LEFT:
				// when selecting a letter which was already selected, we have to shift further the cursor to the left to selet a letter which has not been selected yet
				cursorPosition.col = findUnSelectedLetterIndex(false);
				break;
			case BUTTON_CENTER:
				// identifying the selected letter by cursor position, check if it is in the word - if so add to the "good guesses"
				chosenChar = 
					(cursorPosition.col < LINE_CAPACITY_LETTERCHOOSER) ? LETTERS_FIRST[cursorPosition.col - 1] : LETTERS_SECOND[(cursorPosition.col % LINE_CAPACITY_LETTERCHOOSER) - 1];
				if(strchr(chosenWord, chosenChar) != NULL){
					goodGuesses[strlen(goodGuesses)] = chosenChar;
					goodGuesses[strlen(goodGuesses) + 1] = '\0';
				}
				numberOfGuesses++;
				// reason for this (minus one or two) can be found in comment - check findUnSelectedLetterIndex()
				lettersUsed[cursorPosition.col - (cursorPosition.col < LINE_CAPACITY_LETTERCHOOSER ? 1 : 2)] = true;
				// shifing cursor to the right to select a letter have not been chosen yet, shifing to the left if this is not possible
				int shRightPos = findUnSelectedLetterIndex(true);
				cursorPosition.col = shRightPos == cursorPosition.col ? findUnSelectedLetterIndex(false) : shRightPos;
				// have to check if all letters (and so the word itself) are guessed - if so game is over with a win
				// have to check if the game is lost - not won and no more attempts left
				userInteraction.gameState = WordHasBeenGuessed() ? GAME_OVER_WON : ((BadGuesses() < MAX_BAD_GUESS_ATTEMPTS) ? GAME_NOT_OVER_YET : GAME_OVER_LOST);
				break;
		}
		userInteraction.updateUI = true;
	}

	return userInteraction;
}

void GuessWord(){
	cursorPosition.col = 1;
	cursorPosition.row = 0;
	numberOfGuesses = 0;
	goodGuesses[numberOfGuesses] = '\0';

	int index = 0;
	while((char)(index + 'A') <= 'Z')
		lettersUsed[index++] = false;

	HangmanAreaInit();
	ReDrawGuessWordUI();

	interaction userInteraction = {.updateUI = false, .gameState = GAME_NOT_OVER_YET };
	while(userInteraction.gameState == GAME_NOT_OVER_YET){
		userInteraction = HandleGuessWordBtnPress();
		if(userInteraction.updateUI){
			ReDrawGuessWordUI();
		}
		LCDSendCommand(DD_RAM_ADDR2 + 6);
		LCDSendChar('T');
		LCDSendChar((char)('0' + numberOfGuesses));
		LCDSendChar('G');
		LCDSendChar((char)('0' + strlen(goodGuesses)));
		LCDSendChar('B');
		LCDSendChar((char)('0' + BadGuesses()));
	}
	gameResult = userInteraction.gameState;
}

void GameOverScreen(unsigned char* message){
	LCDSendCommand(CLR_DISP);

	HangmanAreaInit();
	SetHangmanState(BadGuesses());

	char pressCenterBtn[] = "PRESS CENTER";
	int index;
	LCDSendCommand(DD_RAM_ADDR);
	for(index = 0; index < strlen(message); index++)
		LCDSendChar(message[index]);
	LCDSendCommand(DD_RAM_ADDR2);
	for(index = 0; index < strlen(pressCenterBtn); index++)
		LCDSendChar(pressCenterBtn[index]);
}

int main()
{
	PortInit();
	LCDInit();

	while(1){
		SetDisplayContent("Hangman game", "by Viktor");
		WaitForBtnPress(BUTTON_CENTER);

		ChooseWord();
		SetDisplayContent("Word chosen is:", chosenWord);
		WaitForBtnPress(BUTTON_CENTER);

		GuessWord();
		GameOverScreen(gameResult == GAME_OVER_WON ? "YOU WON" : (gameResult ==  GAME_OVER_LOST ? "YOU LOST" : "WTF"));
		WaitForBtnPress(BUTTON_CENTER);
	}
	
	return 0;
}
