// MeasurementOfDistance.c
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to periodically initiate a software-
// triggered ADC conversion, convert the sample to a fixed-
// point decimal distance, and store the result in a mailbox.
// The foreground thread takes the result from the mailbox,
// converts the result to a string, and prints it to the
// Nokia5110 LCD.  The display is optional.
// January 15, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// Slide pot pin 3 connected to +3.3V
// Slide pot pin 2 connected to PE2(Ain1) and PD3
// Slide pot pin 1 connected to ground


#include "ADC.h"
#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "TExaS.h"

void EnableInterrupts(void);  // Enable interrupts

unsigned char String[10]; // null-terminated ASCII string
unsigned long Distance;   // units 0.001 cm
unsigned long ADCdata;    // 12-bit 0 to 4095 sample
unsigned long Flag;       // 1 means valid Distance, 0 means Distance is empty

//********Convert****************
// Convert a 12-bit binary ADC sample into a 32-bit unsigned
// fixed-point distance (resolution 0.001 cm).  Calibration
// data is gathered using known distances and reading the
// ADC value measured on PE1.  
// Overflow and dropout should be considered 
// Input: sample  12-bit ADC sample
// Output: 32-bit distance (resolution 0.001cm)
unsigned long Convert(unsigned long sample){
	unsigned long value;
	value = ((sample*153)>>10)+1;
  return value;  // replace this line with real code
}

// Initialize SysTick interrupts to trigger at 40 Hz, 25 ms
void SysTick_Init(unsigned long period){
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = 0x07;
  EnableInterrupts();

}
// executes every 25 ms, collects a sample, converts and stores in mailbox
void SysTick_Handler(void){ 
	ADCdata = ADC0_In();
	Distance = Convert(ADCdata);
	Flag = 0x01;
}

//-----------------------UART_ConvertDistance-----------------------
// Converts a 32-bit distance into an ASCII string
// Input: 32-bit number to be converted (resolution 0.001cm)
// Output: store the conversion in global variable String[10]
// Fixed format 1 digit, point, 3 digits, space, units, null termination
// Examples
//    4 to "0.004 cm"  
//   31 to "0.031 cm" 
//  102 to "0.102 cm" 
// 2210 to "2.210 cm"
//10000 to "*.*** cm"  any value larger than 9999 converted to "*.*** cm"
void UART_ConvertDistance(unsigned long n){
	int i = 0;
	
	if (n>9999) {
		String[0] = '*';
		String[1] = '*';
		String[2] = '*';
		String[3] = '*';
		String[4] = ' ';
		String[5] = 0;
	}
	else {
		String[0] = n/1000 + 0x30;	// thousands digit
		n = n%1000;									// n is now between 0 and 999
		String[1] = n/100 + 0x30;		// hundreds digit
		n = n%100;									// n is now between 0 and 99
		String[2] = n/10 + 0x30;		// tens digit
		n = n%10;
		String[3] = n + 0x30;				// ones digit
		String[4] = ' ';
		String[5] = 0;
		
		while (i != 3 && String[i] == '0') {
			String[i] = ' ';
			i++;
		}
	}
}

// main1 is a simple main program allowing you to debug the ADC interface
//int main(void){ 
//  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
//  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
//  EnableInterrupts();
//  while(1){ 
//    ADCdata = ADC0_In();
//  }
//}
// once the ADC is operational, you can use main2 to debug the convert to distance
//int main(void){ 
//  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_NoScope);
//  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
//  Nokia5110_Init();             // initialize Nokia5110 LCD
//  EnableInterrupts();
//  while(1){ 
//    ADCdata = ADC0_In();
//    Nokia5110_SetCursor(0, 0);
//    Distance = Convert(ADCdata);
//    UART_ConvertDistance(Distance); // from Lab 11
//    Nokia5110_OutString(String);    // output to Nokia5110 LCD (optional)
//  }
//}
// once the ADC and convert to distance functions are operational,
// you should use this main to build the final solution with interrupts and mailbox
int main(void){ 
  volatile unsigned long delay;
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
// initialize ADC0, channel 1, sequencer 3
	ADC0_Init();
// initialize Nokia5110 LCD (optional)
	Nokia5110_Init();
// initialize SysTick for 40 Hz interrupts
	SysTick_Init(0x1E8480);
// initialize profiling on PF1 (optional)
                                    //    wait for clock to stabilize

  EnableInterrupts();
// print a welcome message  (optional)
  while(1){ 
// read mailbox
		Flag = 0x00;
		while (Flag == 0x01) {
			;
		}
		UART_ConvertDistance(Distance);
// output to Nokia5110 LCD (optional)
		Nokia5110_SetCursor(0, 0);
		Nokia5110_OutString(String);
  }
}
