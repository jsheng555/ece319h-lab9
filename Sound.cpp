// Sound.cpp
// Runs on MSPM0
// Sound assets in sounds/sounds.h
// Jonathan Valvano
// 11/15/2021 
// Jeremy Sheng, Dhilan Nag
// Apr 12 2024
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "Sound.h"
#include "sounds/sounds.h"
#include "../inc/DAC5.h"
#include "../inc/Timer.h"


#define PB0INDEX  11 // UART0_TX  SPI1_CS2  TIMA1_C0  TIMA0_C2
#define PB1INDEX  12 // UART0_RX  SPI1_CS3  TIMA1_C1  TIMA0_C2N
#define PB2INDEX  14 // UART3_TX  UART2_CTS I2C1_SCL  TIMA0_C3  UART1_CTS TIMG6_C0  TIMA1_C0
#define PB3INDEX  15 // UART3_RX  UART2_RTS I2C1_SDA  TIMA0_C3N UART1_RTS TIMG6_C1  TIMA1_C1
#define PB4INDEX  16 // UART1_TX  UART3_CTS TIMA1_C0  TIMA0_C2  TIMA1_C0N
#define DACBITS   31



void SysTick_IntArm(uint32_t period, uint32_t priority){
    // write this
    SysTick->CTRL = 0;         // disable SysTick during setup
    SysTick->LOAD = period-1;  // reload value
    SysTick->VAL = 0;          // any write to current clears it
    SCB->SHP[1] = (SCB->SHP[1]&(~0xC0000000))|(priority << 29); // priority 2 // 30??
    SysTick->CTRL = 0x07;      // enable SysTick with core clock and interrupts
}




// initialize a 11kHz SysTick, however no sound should be started
// initialize any global variables
// Initialize the 5 bit DAC
uint8_t soundIndex = 0;
int soundSize = 0;
int i = 0;
void Sound_Init(void){
// write this (INCOMPLETE)

    // initialize an 11kHz SysTick
    SysTick_IntArm(17273, 2);
    // global variables?


    // initialize 5 bit DAC
    DAC5_Init();

    // my code to attempt background music
    // TimerG0_IntArm(20000,40,2); // 40MHz/40/200000 = 50Hz

}
extern "C" void SysTick_Handler(void);
void SysTick_Handler(void){ // called at 11 kHz
  // output one value to DAC if a sound is active
    if (i > soundSize) {
        SysTick->LOAD = 0;
    }
    switch(soundIndex)
    {
    case 0:
        DAC5_Out(button[i++]);
        break;
    case 1:
        DAC5_Out(jump[i++]);
        break;
    }
}

//******* Sound_Start ************
// This function does not output to the DAC. 
// Rather, it sets a pointer and counter, and then enables the SysTick interrupt.
// It starts the sound, and the SysTick ISR does the output
// feel free to change the parameters
// Sound should play once and stop
// Input: pt is a pointer to an array of DAC outputs
//        count is the length of the array
// Output: none
// special cases: as you wish to implement
void Sound_Start2(const uint8_t pt, uint32_t count){
// write this
    SysTick->LOAD = 7256-1;
    i = 0;
    soundIndex = pt;
    soundSize = count;
}
void Sound_Button(void){
// write this
    Sound_Start2(0, 5472);
}
void Sound_Jump(void){
// write this
    Sound_Start2(1, 3168);
  
}
void Sound_Explosion(void){
// write this
  
}

void Sound_Fastinvader1(void){
  
}
void Sound_Fastinvader2(void){
  
}
void Sound_Fastinvader3(void){
  
}
void Sound_Fastinvader4(void){
  
}
void Sound_Highpitch(void){
  
}
