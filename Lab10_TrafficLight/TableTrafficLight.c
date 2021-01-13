// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"

#include "SysTick.h"

#define LIGHT                   (*((volatile unsigned long *)0x400050FC))
#define GPIO_PORTB_OUT          (*((volatile unsigned long *)0x400050FC)) // bits 5-0
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
#define GPIO_PORTE_IN           (*((volatile unsigned long *)0x400243FC)) // bits 2-0
#define SENSOR                  (*((volatile unsigned long *)0x400243FC))
	
#define LIGHT_PEDESTRIAN				(*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_OUT          (*((volatile unsigned long *)0x400253FC))	// bits 3 and 1
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))

// ***** 2. Global Declarations Section *****

// Linked data structure
struct State {
	unsigned long Pedestrian;
	unsigned long Out;
	unsigned long Time;
	unsigned long Next[8];};

typedef const struct State STyp;
#define GoWest 			0
#define WaitWest		1
#define GoSouth			2
#define WaitSouth		3
#define GoWalk			4
#define WalkFlash1	5
#define WalkFlash2	6
#define WalkFlash3	7
#define WalkFlash4	8
#define WalkFlash5	9
#define WalkFlash6	10
#define NoWalk			11
	
STyp FSM[12]={
	{0x2,0x0c,100,{GoWest,GoWest,WaitWest,WaitWest,WaitWest,WaitWest,WaitWest,WaitWest}},
  {0x2,0x14,50,{GoWalk,GoWest,GoSouth,GoSouth,GoWalk,GoWalk,GoWalk,GoWalk}},
	{0x2,0x21,100,{GoSouth,WaitSouth,GoSouth,WaitSouth,WaitSouth,WaitSouth,WaitSouth,WaitSouth}},
	{0x2,0x22,50,{GoWest,GoWest,GoSouth,GoWest,GoWalk,GoWest,GoWalk,GoWest}},
	{0x8,0x24,100,{GoWalk,WalkFlash1,WalkFlash1,WalkFlash1,GoWalk,WalkFlash1,WalkFlash1,WalkFlash1}},
	{0x2,0x24,20,{WalkFlash2,WalkFlash2,WalkFlash2,WalkFlash2,WalkFlash2,WalkFlash2,WalkFlash2,WalkFlash2}},
	{0x0,0x24,20,{WalkFlash3,WalkFlash3,WalkFlash3,WalkFlash3,WalkFlash3,WalkFlash3,WalkFlash3,WalkFlash3}},
	{0x2,0x24,20,{WalkFlash4,WalkFlash4,WalkFlash4,WalkFlash4,WalkFlash4,WalkFlash4,WalkFlash4,WalkFlash4}},
	{0x0,0x24,20,{WalkFlash5,WalkFlash5,WalkFlash5,WalkFlash5,WalkFlash5,WalkFlash5,WalkFlash5,WalkFlash5}},
	{0x2,0x24,20,{WalkFlash6,WalkFlash6,WalkFlash6,WalkFlash6,WalkFlash6,WalkFlash6,WalkFlash6,WalkFlash6}},
	{0x0,0x24,20,{NoWalk,NoWalk,NoWalk,NoWalk,NoWalk,NoWalk,NoWalk,NoWalk}},
	{0x2,0x24,100,{GoSouth,GoWest,GoSouth,GoSouth,GoWalk,GoWest,GoSouth,GoSouth}}
};

unsigned long S;		// index to the current state
unsigned long Input;

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

// ***** 3. Subroutines Section *****

int main(void){ volatile unsigned long delay;
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
 
	
	SysTick_Init();
  SYSCTL_RCGC2_R |= 0x32;      // 1) B E F
  delay = SYSCTL_RCGC2_R;      // 2) no need to unlock
  GPIO_PORTE_AMSEL_R &= ~0x07; // 3) disable analog function on PE2-0
  GPIO_PORTE_PCTL_R &= ~0x00000FFF; // 4) enable regular GPIO
  GPIO_PORTE_DIR_R &= ~0x07;   // 5) inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= ~0x07; // 6) regular function on PE2-0
  GPIO_PORTE_DEN_R |= 0x07;    // 7) enable digital on PE2-0
  GPIO_PORTB_AMSEL_R &= ~0x3F; // 3) disable analog function on PB5-0
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
  GPIO_PORTB_DIR_R |= 0x3F;    // 5) outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F; // 6) regular function on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;    // 7) enable digital on PB5-0
	
	//SYSCTL_RCGCGPIO_R					// Enable the clock to the port by setting the appropriate bit in the RCGCGPIO
	//PF3 and PF1
	GPIO_PORTF_AMSEL_R &= ~0x0A;					// disable analog function on PF3 and PF1
	GPIO_PORTF_PCTL_R &= ~0x0000F0F0; 		// enable regular GPIO
	GPIO_PORTF_DIR_R |= 0x0A;   					// outputs on PF3 and PF1
	GPIO_PORTF_AFSEL_R &= ~0x0A;					// regular function on PF3 and PF1
	GPIO_PORTF_DEN_R |= 0x0A;							// enable digital on PF3 and PF1
	
  S = WalkFlash1; 
	
  
  EnableInterrupts();
  while(1){
		LIGHT_PEDESTRIAN = FSM[S].Pedestrian;
    LIGHT = FSM[S].Out;  // set lights
    SysTick_Wait10ms(FSM[S].Time);
    Input = SENSOR;     // read sensors
    S = FSM[S].Next[Input];  
  }
}

