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

}
// initialize a 11kHz SysTick, however no sound should be started
// initialize any global variables
// Initialize the 5 bit DAC
void Sound_Init(void){
// write this (INCOMPLETE)

    // initialize an 11kHz SysTick

    // global variables?


    // initialize 5 bit DAC
    GPIOB->DOE31_0 |= (0b11111);
    IOMUX->SECCFG.PINCM[PB0INDEX] = 0x81;
    IOMUX->SECCFG.PINCM[PB1INDEX] = 0x81;
    IOMUX->SECCFG.PINCM[PB2INDEX] = 0x81;
    IOMUX->SECCFG.PINCM[PB3INDEX] = 0x81;
    IOMUX->SECCFG.PINCM[PB4INDEX] = 0x81;
}
extern "C" void SysTick_Handler(void);
void SysTick_Handler(void){ // called at 11 kHz
  // output one value to DAC if a sound is active
   
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
void Sound_Start(const uint8_t *pt, uint32_t count){
// write this
  
}
void Sound_Shoot(void){
// write this
  
}
void Sound_Killed(void){
// write this
  
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
