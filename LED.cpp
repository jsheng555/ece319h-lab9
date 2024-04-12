/*
 * LED.cpp
 *
 *  Created on: Apr 11, 2024
 *  Author: Jeremy Sheng
 *  MAKE SURE JUMPERS 16,17,18 ARE DISCONNECTED TO UTILIZE PA26
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table
#define redLED (1<<26)
#define yellowLED (1<<25)
#define greenLED (1<<24)

// initialize your LEDs
void LED_Init(void){
    // write this
    GPIOA->DOE31_0 |= redLED + yellowLED + greenLED;
    IOMUX->SECCFG.PINCM[PA26INDEX] = 0x81;
    IOMUX->SECCFG.PINCM[PA25INDEX] = 0x81;
    IOMUX->SECCFG.PINCM[PA24INDEX] = 0x81;
}
// data specifies which LED to turn on
// 0 is red ; 1 is yellow ; 2 is green
void LED_On(uint32_t data){
    // write this
    // use DOUTSET31_0 register so it does not interfere with other GPIO
    if (data==0) GPIOA->DOUTSET31_0 = redLED;
    if (data==1) GPIOA->DOUTSET31_0 = yellowLED;
    if (data==2) GPIOA->DOUTSET31_0 = greenLED;
}

// data specifies which LED to turn off
// 0 is red ; 1 is yellow ; 2 is green
void LED_Off(uint32_t data){
    // write this
    // use DOUTCLR31_0 register so it does not interfere with other GPIO
    if (data==0) GPIOA->DOUTCLR31_0 = redLED;
    if (data==1) GPIOA->DOUTCLR31_0 = yellowLED;
    if (data==2) GPIOA->DOUTCLR31_0 = greenLED;
}

// data specifies which LED to toggle
// 0 is red ; 1 is yellow ; 2 is green
void LED_Toggle(uint32_t data){
    // write this
    // use DOUTTGL31_0 register so it does not interfere with other GPIO
    if (data==0) GPIOA->DOUTTGL31_0 = redLED;
    if (data==1) GPIOA->DOUTTGL31_0 = yellowLED;
    if (data==2) GPIOA->DOUTTGL31_0 = greenLED;
}
