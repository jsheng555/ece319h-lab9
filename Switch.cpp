/*
 * Switch.cpp
 *
 *  Created on: Apr 11, 2024
 *      Author: Jeremy Sheng, Dhilan Nag
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"

#define upSwitch (1<<19)
#define leftSwitch (1<<16)
#define downSwitch (1<<17)
#define rightSwitch (1<<18)
// LaunchPad.h defines all the indices into the PINCM table
void Switch_Init(void){
    // write this
    GPIOA->DOE31_0 |= leftSwitch + rightSwitch + downSwitch;
    GPIOB->DOE31_0 |= upSwitch;
    IOMUX->SECCFG.PINCM[PA16INDEX] = 0x40081;
    IOMUX->SECCFG.PINCM[PA17INDEX] = 0x40081;
    IOMUX->SECCFG.PINCM[PA18INDEX] = 0x40081;
    IOMUX->SECCFG.PINCM[PB19INDEX] = 0x40081;
}
// return current state of switches
// bit 0 = right switch
// bit 1 = up switch
// bit 2 = left switch
// bit 3 = down switch
uint32_t Switch_In(void){
    // write this
    uint32_t input = 0;
    input += (GPIOA->DIN31_0 & rightSwitch)?1:0;
    input += (GPIOB->DIN31_0 & upSwitch)?2:0;
    input += (GPIOA->DIN31_0 & leftSwitch)?4:0;
    input += (GPIOA->DIN31_0 & downSwitch)?8:0;
}
