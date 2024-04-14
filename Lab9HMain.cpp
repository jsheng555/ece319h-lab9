// Lab9HMain.cpp
// Runs on MSPM0G3507
// Lab 9 ECE319H
// Jeremy Sheng, Dhilan Nag
// Last Modified: 1/1/2024

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/SlidePot.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
extern "C" void __disable_irq(void);
extern "C" void __enable_irq(void);
extern "C" void TIMG12_IRQHandler(void);
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}

SlidePot Sensor(1500,0); // copy calibration from Lab 7

int phase = 0; // 0 = welcome, 1 = gameplay, 2 = lose

void DrawStars() {
    if (phase == 0) {
        for (int i = 0 ; i < 22 ; i++) {
            ST7735_DrawPixel(Random32()%64, Random32()%110 + 50, ST7735_Color565(255,255,255));
        }
        for (int i = 0 ; i < 22 ; i++) {
            ST7735_DrawPixel((Random32()%64)+64, Random32()%110 + 50, ST7735_Color565(255,255,255));
        }
    } else if (phase == 1) {
        for (int i = 0 ; i < 22 ; i++) {
            ST7735_DrawPixel(Random32()%64, Random32()%160, ST7735_Color565(255,255,180));
        }
        for (int i = 0 ; i < 22 ; i++) {
            ST7735_DrawPixel((Random32()%64)+64, Random32()%160, ST7735_Color565(255,255,180));
        }
    }

}

void DrawRoad() {
    for (int i = 0 ; i < 128 ; i++) {
        ST7735_Line(64, 0, i, 150, 0xFFFF);
        ST7735_Line(i,150,i,160,0xFFFF);
    }
    //ST7735_FillRect(0,150,128,159,0xFFFF);
}

// known to work
// color 0 = red, 1 = white
void DrawSpike(int x, int y, int color) {
    int height = y/7 + 1; // divisor is arbitrary. controls size of spike.
    for (int i = y ; i < y+height * 1.4 ; i++) {
        for (int j = x-(i-y)/2 ; j <= x+(i-y)/2 ; j++) {
            if ((j-x)*(j-x) + (i-y-height/2)*(i-y-height/2) > height*height/2 && i >= y+height) continue;
            if (color == 0) {
                if (j<x - (i-y)/4) {
                    ST7735_DrawPixel(j, i, ST7735_DARKRED);
                } else if (j>x+(i-y)/4) {
                    ST7735_DrawPixel(j, i, ST7735_LIGHTERRED);
                } else {
                    ST7735_DrawPixel(j, i, ST7735_LIGHTRED);
                }
            } else {
                ST7735_DrawPixel(j, i, ST7735_WHITE);
            }

        }
    }
    // an attempt to make the spikes rounder
//    if (color == 0) {
//        ST7735_Line(x-height/2.7+1, y+height, x+height/2.7, y+height, ST7735_LIGHTRED);
//    } else {
//        ST7735_Line(x-height/2.7+1, y+height, x+height/2.7, y+height, ST7735_WHITE);
//    }

}

#define MAXSPIKES 20
int spikeArray[MAXSPIKES][3]; // [spikeIndex][spikeExists?,X-trajectory,y-position]
int putSpikeIndex = 0;

void spawnSpike() {
    int xtrajectory = Random32()%128;
    spikeArray[putSpikeIndex][0] = 1;
    spikeArray[putSpikeIndex][1] = xtrajectory;
    spikeArray[putSpikeIndex][2] = 1;
    putSpikeIndex++;
    putSpikeIndex %= MAXSPIKES;
}

void moveSpikes() {

    // undraw old spikes, update spike array, and draw new spikes
    for (int i = 0 ; i < MAXSPIKES ; i++) {
        if (spikeArray[i][0] == 0) continue;

        // undraw spikes
        int trajectory = spikeArray[i][1];
        int currY = spikeArray[i][2];
        DrawSpike(64 + (currY * (trajectory - 64) / 160), currY, 1);

        // update spike array
        if (currY > 160) {
            spikeArray[i][0] = 0;
            continue;
        }

        // draw new spikes
        currY += 1 + currY/50;
        spikeArray[i][2] = currY;
        DrawSpike(64 + (currY * (trajectory - 64) / 160), currY, 0);

    }

}

int flag = 0;
int prevplayerX = 0;
int playerX = 0;
int switchin = 0;
int LEDindex = 0;
int LEDslowdown = 0;
// phase = 0; // 0 = welcome, 1 = gameplay, 2 = lose
// GAME ENGINE
// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here
    // 1) sample slide pot
    int slidepot = Sensor.In();
    // 2) read input switches
    switchin = Switch_In();
    // 3) move sprites
    prevplayerX = playerX;
    playerX = slidepot/48 + 14;

    // 4) start sounds
    // 5) set semaphore
    flag = 1;

    // LEDs
    if (LEDslowdown++%5 == 0 && phase==0) LED_Toggle(LEDindex++%3);

    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

typedef enum {English, Italian, Klingon} Language_t;
Language_t myLanguage=English;
typedef enum {HELLO, GOODBYE, LANGUAGE} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Italian[] ="Ciao";
const char Hello_Klingon[] ="nuqneH";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Italian[]="Arrivederci";
const char Goodbye_Klingon[] = "idk";
const char Language_English[]="English "; // make sure there are enough spaces to override
const char Language_Italian[]="Italiano";
const char Language_Klingon[]="Klingon ";
const char Start_English[]="Press Up to Start \n               "; // make sure there are enough spaces to override
const char Start_Italian[]="   Premi Su per   \n       Iniziare";
const char Start_Klingon[]="      yIvum       \n               ";
const char *Phrases[4][3]={
  {Hello_English,Hello_Italian,Hello_Klingon},
  {Goodbye_English,Goodbye_Italian,Goodbye_Klingon},
  {Language_English,Language_Italian,Language_Klingon},
  {Start_English,Start_Italian,Start_Klingon}
};
// use main1 to observe special characters
int main1(void){ // main1
    char l;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
  ST7735_FillScreen(0x0000);            // set screen to black
  for(int myPhrase=0; myPhrase<= 2; myPhrase++){
    for(int myL=0; myL<= 3; myL++){
         ST7735_OutString((char *)Phrases[LANGUAGE][myL]);
      ST7735_OutChar(' ');
         ST7735_OutString((char *)Phrases[myPhrase][myL]);
      ST7735_OutChar(13);
    }
  }
  Clock_Delay1ms(3000);
  ST7735_FillScreen(0x0000);       // set screen to black
  l = 128;
  while(1){
    Clock_Delay1ms(2000);
    for(int j=0; j < 3; j++){
      for(int i=0;i<16;i++){
        ST7735_SetCursor(7*j+0,i);
        ST7735_OutUDec(l);
        ST7735_OutChar(' ');
        ST7735_OutChar(' ');
        ST7735_SetCursor(7*j+4,i);
        ST7735_OutChar(l);
        l++;
      }
    }
  }
}

// use main2 to observe graphics
int main2(void){ // main2
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_DrawBitmap(20, 100, AlienMiddle, 15, 30);
  DrawRoad();
//  ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); // player ship bottom
//  ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
//  ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); // player ship bottom
//  ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); // player ship bottom
//  ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); // player ship bottom
//  ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
//  ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
//  ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
//  ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
//  ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);

  for(uint32_t t=500;t>0;t=t-0){
    SmallFont_OutVertical(t,104,6); // top left
    Clock_Delay1ms(50);              // delay 50 msec
  }
  ST7735_FillScreen(0x0000);   // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString((char *)"GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_OutString((char *)"Nice try,");
  ST7735_SetCursor(1, 3);
  ST7735_OutString((char *)"Earthling!");
  ST7735_SetCursor(2, 4);
  ST7735_OutUDec(1234);
  while(1){
  }
}

// use main3 to test switches and LEDs
int main3(void){ // main3
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  Sensor.Init();

  ST7735_InitPrintf();
  ST7735_FillScreen(ST7735_BLACK);
  while(1){
    // write code to test switches and LEDs
    // read switches and output to lcd
    int switchInput = Switch_In();
    ST7735_SetCursor(1, 1);
    ST7735_OutChar(switchInput/10 + 0x30);
    ST7735_OutChar(switchInput%10 + 0x30);
    ST7735_SetCursor(1, 2);
    int slidein = Sensor.Convert(Sensor.In());
    ST7735_OutChar(slidein/1000 + 0x30);
    ST7735_OutChar('.');
    slidein %= 1000;
    ST7735_OutChar(slidein/100 + 0x30);
    slidein %= 100;
    ST7735_OutChar(slidein/10 + 0x30);
    slidein %= 10;
    ST7735_OutChar(slidein/1 + 0x30);

    // randomly toggle all 3 LEDs
    if (Random32()%1000==0) LED_Toggle(0);
    if (Random32()%1000==0) LED_Toggle(1);
    if (Random32()%1000==0) LED_Toggle(2);
  }
}
// use main4 to test sound outputs
int main4(void){ uint32_t last=0,now;
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  LED_Init(); // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(ADC0,6,0); // ADC1 channel 6 is PB20, TExaS scope
  __enable_irq();
  while(1){
    now = Switch_In(); // one of your buttons
    if((last == 0)&&(now == 1)){
      Sound_Shoot(); // call one of your sounds
    }
    if((last == 0)&&(now == 2)){
      Sound_Killed(); // call one of your sounds
    }
    if((last == 0)&&(now == 4)){
      Sound_Explosion(); // call one of your sounds
    }
    if((last == 0)&&(now == 8)){
      Sound_Fastinvader1(); // call one of your sounds
    }
    // modify this to test all your sounds
  }
}

int langIndex = 0;
void drawLangStart() {
  ST7735_SetCursor(2,8);
  ST7735_OutString((char *)Phrases[3][langIndex]);
  ST7735_SetCursor(7,14);
  ST7735_OutString((char *)Phrases[2][langIndex]);
}
void switchLanguage() {
    if (switchin == 1) {
      if (myLanguage == English) {
          myLanguage = Italian;
          langIndex = 1;
          while (switchin == 1){};
      }
      else if (myLanguage == Italian) {
          myLanguage = Klingon;
          langIndex = 2;
          while (switchin == 1){};
      }
      else if (myLanguage == Klingon) {
          myLanguage = English;
          langIndex = 0;
          while (switchin == 1){};
      }
      // ST7735_FillRect(0,75,128,24,ST7735_BLACK);
      drawLangStart();
  }
  if (switchin == 4) {
    if (myLanguage == English) {
        myLanguage = Klingon;
        langIndex = 2;
        while (switchin == 4){};
    }
    else if (myLanguage == Italian) {
        myLanguage = English;
        langIndex = 0;
        while (switchin == 4){};
    }
    else if (myLanguage == Klingon) {
        myLanguage = Italian;
        langIndex = 1;
        while (switchin == 4){};
    }
    // ST7735_FillRect(0,75,128,24,ST7735_BLACK);
    drawLangStart();
  }
}

void drawScore(int sc) {
    if (sc <= 9) {
        ST7735_OutChar(sc + 0x30);
    } else {
        drawScore(sc/10);
        ST7735_OutChar((sc % 10) + 0x30);
    }
}



// ALL ST7735 OUTPUT MUST OCCUR IN MAIN
int main(void){ // final main
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf();
    //note: if you colors are weird, see different options for
    // ST7735_InitR(INITR_REDTAB); inside ST7735_InitPrintf()
  ST7735_FillScreen(ST7735_BLACK);
  Sensor.Init(); // PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init();  // initialize sound
  TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(80000000/30,2);
  // initialize all data structures
  int score = 0;
  __enable_irq();

  // Start screen. Phase 0
  phase = 0;
  ST7735_DrawBitmap(5,59,Logo,118,50);
  DrawStars();
  drawLangStart();
  while (1) {
      // wait for semaphore
      while (flag == 0) {}
      // clear semaphore
      flag = 0;
      // game start?
      if (switchin == 2) {
          ST7735_FillScreen(ST7735_BLACK);
          break; // move to phase 1
      }
      // language change?
      switchLanguage();
  }

  phase = 1;
  LED_Off(0); LED_Off(1); LED_Off(2);

  DrawStars();
  DrawRoad();

  while(1){
    // wait for semaphore
      while (flag == 0) {}
       // clear semaphore
      flag = 0;
       // update ST7735R
      ST7735_DrawBitmap(playerX, 150, AlienMiddle, 15, 30);
      if (Random32() % 30 == 0) spawnSpike();
      moveSpikes();

      // draw player TODO: optimize
      // idea: have the player's bitmap have wide white background. redraw the small black triangles
      // over the white bitmap rather than redrawing the large road actually i don't like this idea!
      // another idea: same as above but limit the white background to ±5 pixels on each side. Limit
      // the player to move max 5 pixels per frame, so the 5 pixel margin is sufficient.
      if (abs(prevplayerX - playerX) >= 1) {
          ST7735_FillRect(12, 121, 104, 40, 0xFFFF);
          ST7735_DrawBitmap(playerX, 150, AlienMiddle, 15, 30);
      }

      // draw score TODO: make score increase every 10 frames not 1 using SysTick
      score += 10;
      ST7735_SetCursor(0,0);
      drawScore(score);

    // check for end game or level switch
  }
}
