/* This sketch is for the LiPo Monitor Prototype.  It is intended to detect the
 * power level of the lowest cell in a 4S LiPo and determine whether or not the
 * battery is balanced or not.  The battery level will be displayed using four
 * LEDs on the board.  If the battery is unbalanced, the LEDs will blink.  If
 * the battery is unbalanced and empty, the LEDs will blink in a distinctive
 * two on two off fashion.
 */

#define PWR25 7	// 25% power level LED pin
#define PWR50 8	// 50% power level LED pin
#define PWR75 2	// 75% power level LED pin
#define PWR10 4	// 100% power level LED pin
#define CELL1 0	// Cell1 definition for array reference in cell[]
#define CELL2 1	// Cell2 definition for array reference in cell[]
#define CELL3 2	// Cell3 definition for array reference in cell[]
#define CELL4 3	// Cell4 definition for array reference in cell[]
#define BATT 4	// Battery definition (total voltage) for array reference in cell[]

#define CELL1SCALE 203.6	// Scaling factor for cell 1 (from ADC to real voltage)
#define CELL2SCALE 94.39596667	// Scaling factor for cell 2
#define CELL3SCALE 68.02229969	// Scaling factor for cell 3
#define CELL4SCALE 50.46576421	// Scaling factor for cell 4

#define batBal 0.2	// total allowable difference in voltage between max and min cells

float cell[5];	// battery cell voltages
String inputString = "";	// a string to hold incoming data
int lowCell = 0;	// index of the lowest cell voltage
int maxCell = 0;	// index of the highest cell voltage
int stateMask = 0;	// state machine bit pattern.  Big endian.
// Intended 5 bit mask.  first three bits are power level.  Fourth bit is balance state.  Fifth bit is blink state


void setup(){
	Serial.begin(9600);
	pinMode(PWR25, OUTPUT);
	pinMode(PWR50, OUTPUT);
	pinMode(PWR75, OUTPUT);
	pinMode(PWR10, OUTPUT);
	digitalWrite(PWR25, HIGH);
	digitalWrite(PWR50, HIGH);
	digitalWrite(PWR75, HIGH);
	digitalWrite(PWR10, HIGH);	
}

void loop(){
	stateMask = stateMask & 1;	// Zero out first four bits.
	// Gather Data
	cell[CELL1] = analogRead(A0) / CELL1SCALE;	// Scale first cell
	cell[CELL2] = analogRead(A1) / CELL2SCALE - cell[CELL1];	// Scale second reading, subtract cell 1 to get cell 2
	cell[CELL3] = analogRead(A2) / CELL3SCALE - cell[CELL2] - cell[CELL1];	// and so on.
	cell[BATT] = analogRead(A3) / CELL4SCALE;	// Last cell is initially measured as total available voltage.
	cell[CELL4] = cell[BATT] - cell[CELL3] - cell[CELL2] - cell[CELL1];	// Subtract previous cells
	
	// Identify Low Cell and High Cells
	for(int i = 0; i < 4; i++){
		if(cell[lowCell] > cell[i]){
			lowCell = i;
		}
		if(cell[maxCell] < cell[i]){
			maxCell = i;
		}
	}
	
	// Identify power level
	if(cell[lowCell] < 3.775){	// Power Level 0
		stateMask = stateMask | 0;	// Don't change bits
	}
	if(cell[lowCell] >= 3.775 || cell[lowCell] < 3.925){	// Power Level 1
		stateMask = stateMask | 4;	// Set third bit high
	}
	if(cell[lowCell] >= 3.925 || cell[lowCell] < 4.075){	// Power Level 2
		stateMask = stateMask | 8;	// Set second bit high
	}
	if(cell[lowCell] >= 4.075 || cell[lowCell] < 4.225){	// Power Level 3
		stateMask = stateMask | 12;	// Set second and third bits high
	}
	if(cell[lowCell] >= 4.225){	//Power Level 4
		stateMask = stateMask | 16;	// Set first bit high
	}

	// Identify unbalanced cells
	if(cell[maxCell] - cell[lowCell] > batBal){
		stateMask = stateMask | 2;	// Set fourth bit high
	}
	
	// Display State Machine
/*
State Table:

State	Pwr Lvl		Balance		Blinking	Next State	LEDs
=====	=======		=======		========	==========	====
														1234
0		0			0			0			0			    	Empty, but balanced
1		0			0			1			0			    	Error state.  Returns to normal state.
2		0			1			0			3			+ + 	Empty, but unbalanced.  Blink state 1.
3		0			1			1			2			 + +	Empty, but unbalanced.  Blink state 2.
4		1			0			0			4			+   	25% remaining, balanced.
5		1			0			1			4			    	Error state.  Returns to normal state.
6		1			1			0			7			+   	25% remaining, unbalanced.  Blink state 1.
7		1			1			1			6			    	25% remaining, unbalanced.  Blink state 2.
8		2			0			0			8			++  	50% remaining, balanced.
9		2			0			1			8			    	Error state.  Returns to normal state.
10		2			1			0			11			++  	50% remaining, unbalanced.  Blink state 1.
11		2			1			1			10			    	50% remaining, unbalanced.  Blink state 2.
12		3			0			0			12			+++ 	75% remaining, balanced.
13		3			0			1			12			    	Error state.  Returns to normal state.
14		3			1			0			15			+++ 	75% remaining, unbalanced.  Blink state 1.
15		3			1			1			14			    	75% remaining, unbalanced.  Blink state 2.
16		4			0			0			16			++++	100% remaining, balanced.
17		4			0			1			16			    	Error state.  Returns to normal state.
18		4			1			0			19			++++	100% remaining, unbalanced.  Blink state 1.
19		4			1			1			18			    	100% remaining, unbalanced.  Blink state 2.
 */
	switch(stateMask){
		case 0:
			stateMask = 0;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 4:
			stateMask = 4;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 8:
			stateMask = 8;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 12:
			stateMask = 12;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, HIGH);
			digitalWrite(PWR10, LOW);
			break;
		case 16:
			stateMask = 16;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, HIGH);
			digitalWrite(PWR10, HIGH);
			break;
		case 2:
			stateMask = 3;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, HIGH);
			digitalWrite(PWR10, LOW);
			break;
		case 6:
			stateMask = 7;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 10:
			stateMask = 11;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 14:
			stateMask = 15;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, HIGH);
			digitalWrite(PWR10, LOW);
			break;
		case 18:
			stateMask = 19;
			digitalWrite(PWR25, HIGH);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, HIGH);
			digitalWrite(PWR10, HIGH);
		case 1:
			stateMask = 0;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 5:
			stateMask = 4;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 9:
			stateMask = 8;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 13:
			stateMask = 12;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 17:
			stateMask = 16;		
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 3:
			stateMask = 2;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, HIGH);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, HIGH);
			break;
		case 7:
			stateMask = 6;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 11:
			stateMask = 10;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 15:
			stateMask = 14;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
		case 19:
			stateMask = 18;
			digitalWrite(PWR25, LOW);
			digitalWrite(PWR50, LOW);
			digitalWrite(PWR75, LOW);
			digitalWrite(PWR10, LOW);
			break;
	}
	delay(100);
}

void serialEvent(){
	while (Serial.available()) {
		// get the new byte:
		char inChar = (char)Serial.read(); 
		// add it to the inputString:
		inputString += inChar;
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (inChar == '\n') {
			if(inputString.equals("CELL1\n")){
				Serial.println(cell[CELL1], 3);	// Print cell voltage, 3 places
			}
			if(inputString.equals("CELL2\n")){
				Serial.println(cell[CELL2], 3);	// Print cell voltage, 3 places
			}
			if(inputString.equals("CELL3\n")){
				Serial.println(cell[CELL3], 3);	// Print cell voltage, 3 places
			}
			if(inputString.equals("CELL4\n")){
				Serial.println(cell[CELL4], 3);	// Print cell voltage, 3 places
			}
			if(inputString.equals("BATT\n")){
				Serial.println(cell[BATT], 3);	// Print battery voltage, 3 places
			}
			inputString = "";	// clear string
		} 
	}
}
