
#include <LiquidCrystal.h>
#include <avr/pgmspace.h>
#include "dftcons.h"

LiquidCrystal lcd(8,9,10,11,12,13);
// LCD Levels
prog_uint8_t lcd_char[8][8] PROGMEM = {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
{0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x1F},
{0x00,0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F},
{0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F},
{0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F},
{0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F},
{0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F},
{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}
};

uint8_t tlcd_char[8];

byte micPort = 0x02;

unsigned long frame=0;
unsigned long ttime,ttime1,ttime2,ttime3;

// Input signal
int8_t x[DFT_N];
// test signal
// x={124,43,80,43,-24,80,45,5,73,9,52,127,17,23,17,-46,73,75,12,73,3,1,83,-16,-12,29,-25,79
// ,88,-17,25,-25,-37,76,10,0,58,-13,50,68,-49,1,2,-10,111,52,-3,50,-23,21,78,-22,31,62,21,113,54,-33,38,0,39,125,24,40};

// Result Space
int16_t dft_ReX[DFT_Nb2];
int16_t dft_ImX[DFT_Nb2];
uint8_t dftX[DFT_Nb2];
uint8_t lcd_lvl[DFT_Nb2];

uint8_t band,mag,k,i,j;

// ~~~~~~~~~~~~~~ MAIN ~~~~~~~~~~~~~~~~
void setup() {
	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			tlcd_char[j]=pgm_read_byte(&lcd_char[i][j]);
		}
		lcd.createChar(i, tlcd_char);

	}

	lcd.begin(16,2); 
	Serial.begin(19200);
	Serial.println("RDY");

	// Set up ADC
	// Vref = 5V (REFS1:REFS0 = bit8:7 = 01)
	// Left Adjust read (ADLAR = bit6 = 1)
	// Select analog channel  (MUX3:0 = bit3:0)
	ADMUX = 0b01100000;
	ADMUX |= micPort;
	ADCSRB = 0;
	ADCSRA = 0;
	ADCSRA |= 1<<ADEN;	// Enable ADC
	ADCSRA |= 1<<ADSC;	// Start ADC
	ADCSRA |= 1<<ADPS2; // ADPS2:0 = 3bit prescaler set to 64
	ADCSRA |= 1<<ADPS1; 

	for (k=0;k<DFT_Nb2;k++) {
		lcd_lvl[k]=0;
	}
}
 

void loop() {
frame=millis();
ttime = micros();

//~~~~~~~~~~~~~~~~~ SAMPLING ~~~~~~~~~~~~~~~~~~~
	// Select Analog Channel 
	ADMUX |= micPort;

ttime = micros();
for(i = 0;i<DFT_N;i++){
	// Start conversion
	ADCSRA |= 1<<ADSC;
	// Wait till conversion is done
	while(ADCSRA & 1<<ADSC){};
	// Read ADCH. 
	x[i] = (ADCH);
}
// Scale from 0-255 to -128 to +127
// Apply window to reduce spectral leakage
for(i = 0;i<DFT_N;i++){
	x[i]-=128;
	x[i]=(x[i]*(int8_t)pgm_read_byte(&window[i]))>>7;
}
ttime1 = micros();
// int8_t min,max,test;
// while(1){
// 	min=255;
// 	max=0;
// 	ADMUX |= micPort;

// 	for(i = 0;i<100;i++){
// 		ADCSRA |= 1<<ADSC;
// 		while(ADCSRA & 1<<ADSC){};
// 		//test=analogRead(4)>>2;
// 		test = (ADCH);
// 		test -=128;

// 		if(test>max){max=test;}
// 		if(test<min){min=test;}
// 	}
// 	Serial.print("m=");
//     Serial.print(min);
//     Serial.print(" M=");
//     Serial.print(max);
//     Serial.print(" D=");
//     Serial.print(max-min);
//     Serial.println();
//     delay(400);
// };
//~~~~~~~~~~~~~~~~~~ DFT ~~~~~~~~~~~~~~~~~~~~~~~
	for(k=1;k<DFT_Nb2;k++) {
		dft_ReX[k]=0;
		dft_ImX[k]=0;

		for(i =0;i<DFT_N;i++) {
			dft_ReX[k] += ((int8_t)pgm_read_byte(&REx_cons[k][i]) * x[i])>>2;
			if(k<DFT_Nb2-1){
				dft_ImX[k] += -1 * ((int8_t)pgm_read_byte(&IMx_cons[k][i]) * x[i])>>2;
			}
		}
		dft_ReX[k] = dft_ReX[k] >> (5+LOG2_Nb2);
		dft_ImX[k] = dft_ImX[k] >> (5+LOG2_Nb2);
	}
	//dft_ReX[0] = dft_ReX[0] >> 1;
	dft_ReX[DFT_Nb2-1] = dft_ReX[DFT_Nb2-1] >> 1;

	//Magnitude Calculation
	for(k =1;k<DFT_Nb2;k++) {
		dftX[k] = sqrt(dft_ReX[k]*dft_ReX[k] + dft_ImX[k]*dft_ImX[k]);
	}
ttime2 = micros();
//~~~~~~~~~~~~~~~~~~ LCD RENDER ~~~~~~~~~~~~~~~~~~~~~~~
	lcd.clear();
	lcd.setCursor(0,1);
	for(k =0;k<16;k++) {
		//LINEAR 
		mag=dftX[Bands[k]];

		// Band Accumulator
		// band = Bands[k];
		// mag=0;
		// if(k>9){
		// 	for(j=Bands[k-1]+1;j<=band;j++){
		// 		mag+=dftX[j];	
		// 	}
		// } else {
		// 		mag = dftX[band];
		// }

		
		

		// LOG: 18*log2
		mag = pgm_read_byte(&logMag[mag]);
		// Noise adjust
		if(k==0 && mag<80){mag=0;}
		else if (k==1 && mag<42){mag=0;}
		mag = mag>>3;
		
		// chaser
	 	if (mag<lcd_lvl[k]){lcd_lvl[k]--;}
		else {lcd_lvl[k] = mag;}

		if(lcd_lvl[k]>7){
			lcd.setCursor(k,0);
			lcd.write(lcd_lvl[k]-8);
			lcd.setCursor(k,1);
			lcd.write(7);
		} else {
			lcd.write(lcd_lvl[k]);
		}
	}

ttime3 = micros();

	// Serial.println("REX");
	// for(int k =0;k<DFT_Nb2;k++) {Serial.println(dft_ReX[k]);}
	// Serial.println("IMX");
	// for(int k =0;k<DFT_Nb2;k++) {Serial.println(dft_ImX[k]);}

	// Serial.println("DFTX");
	// for(int k =0;k<DFT_Nb2;k++) {Serial.println(dftX[k]);}
	// Serial.println("LCD");
	// for(int k =0;k<DFT_Nb2;k++) {Serial.println(lcd_lvl[k]);}

	// Serial.println("TIME SAMPLING");
	// Serial.println(ttime1-ttime);
	// Serial.println("TIME DFT");
	// Serial.println(ttime2-ttime1);
	// Serial.println("TIME LCD");
	// Serial.println(ttime3-ttime2);
	// Serial.println("TIME FULL");
	// Serial.println(ttime3-ttime);


	// Frame Guard
	while((millis()-frame)<40){	};
// while (1) {};
}