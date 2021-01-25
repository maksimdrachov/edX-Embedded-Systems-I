#include "Switch.h"
#include "..//tm4c123gh6pm.h"

void Switch_Init(void){
	volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x10;      // 1) Port E enable
  delay = SYSCTL_RCGC2_R;      // 2) no need to unlock
  GPIO_PORTE_AMSEL_R &= ~0x03; // 3) disable analog function on PE2-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF; // 4) enable regular GPIO
  GPIO_PORTE_DIR_R &= ~0x03;   // 5) inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= ~0x03; // 6) regular function on PE2-0
  GPIO_PORTE_DEN_R |= 0x03;    // 7) enable digital on PE2-0
  
}


//------------ADC0_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
int Switch_In(void){
	extern int FlagLaser;
	extern int LaserX;
	extern int LaserY;
	extern int ShipX;
	static volatile long int previous = 0x00;
	int result;
  result = (int)(GPIO_PORTE_DATA_R & 0x03);
	
	if ((previous != (result & 0x01)) && (FlagLaser == 0) ) {
		if ((result & 0x01) == 0x00) {
			FlagLaser++;
			LaserX = ShipX+8;
			LaserY = 40;
		}
	}
	previous = (GPIO_PORTE_DATA_R&0x01);
	
  return result;
}
