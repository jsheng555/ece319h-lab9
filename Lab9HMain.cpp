// Lab9HMain.cpp
// Runs on MSPM0G3507
// Lab 9 ECE319H
// Jeremy Sheng, Dhilan Nag
// Last Modified: 1/1/2024

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmath>
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
#include "DrawSprites.h"
#include "images/images.h"
extern "C" void __disable_irq(void);
extern "C" void __enable_irq(void);
extern "C" void TIMG12_IRQHandler(void);
extern "C" void TIMG8_IRQHandler(void);
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
int max(int a, int b) {return a>b?a:b;}
int min(int a, int b) {return a<b?a:b;}

SlidePot Sensor(1500,0); // copy calibration from Lab 7

int phase = 0; // 0 = welcome, 1 = gameplay, 2 = lose


#define BACKGROUNDMUSICSIZE 128
#define MAXSPIKES 6
int spikeCount = 0;
int spikeArray[MAXSPIKES][3]; // [spikeIndex][spikeExists?,X-trajectory,y-position]
int putSpikeIndex = 0;

void spawnSpike() {
    spikeCount++;
    int xtrajectory = Random32()%128;
    spikeArray[putSpikeIndex][0] = 1;
    spikeArray[putSpikeIndex][1] = xtrajectory;
    spikeArray[putSpikeIndex][2] = 1;
    putSpikeIndex++;
    putSpikeIndex %= MAXSPIKES;
}

// NOTE: If you set the spike speed high, DISABLE the spike refresh optimizer in DrawSprites.h
// NOTE2: auto-disabled the refresh optimizer if speed > 1.6
double spikeSpeed = 1.1;
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
            spikeCount--;
            continue;
        }

        // draw new spikes
        currY += 1 + (currY*spikeSpeed/50.00);
        spikeArray[i][2] = currY;
        DrawSpike(64 + (currY * (trajectory - 64) / 160), currY, 0);

    }

}

// global variables used by game engine
int flag = 0;
int prevplayerX = 0;
int playerX = 0;
int prevrelY = 0;
int relativeY = 0;
int yVel = 0;
int switchin = 0;
int LEDindex = 0;
int slowdown = 0;
int collided = 0;
int oneSecond = 0;
int backgroundIndex = 0;
// a table of periods (notes) to play at NPS (notes per second) speed
const int NPS = 4;
#define B3 5062
#define C4 4778
#define c4 4513
#define D4 4267
#define d4 4018
#define E4 3792
#define F4 3581
#define f4 3378
#define G4 3189
#define A4 2841
#define B4 2530
#define C5 2390
#define D5 2129
#define E5 1896
#define XX 1
/*const uint32_t backgroundMusic[BACKGROUNDMUSICSIZE] = { // 24
  A,E,E,A,E,E,B,E,E,B,E,E,C,E,E,C,E,E,D,E,E,D,E,B
};*/
const uint32_t backgroundMusic[BACKGROUNDMUSICSIZE] = { // 128
  E4,G4,B4,C5,D5,C5,B4,G4, // a section
  D4,G4,B4,C5,D5,C5,B4,G4,
  C4,G4,B4,C5,E5,C5,B4,G4,
  D4,G4,B4,C5,D5,C5,B4,G4,
  E4,G4,B4,C5,D5,C5,B4,G4,
  D4,G4,B4,C5,D5,C5,B4,G4,
  C4,G4,B4,C5,E5,C5,B4,G4,
  D4,G4,B4,C5,D5,C5,B4,G4,
  E5,B4,G4,f4,E4,f4,G4,E5, // b section
  D5,B4,f4,E4,D4,E4,f4,D5,
  C5,G4,E4,D4,C4,D4,E4,C5,
  B4,f4,d4,c4,B3,c4,d4,B4,
  E5,B4,G4,f4,E4,f4,G4,E5,
  D5,B4,f4,E4,D4,E4,f4,D5,
  C5,G4,E4,D4,C4,D4,E4,C5,
  B4,f4,d4,c4,B3,B3,XX,XX
};
void detectCollisions() {
    for (int i = 0 ; i < MAXSPIKES ; i++) {
        int spiketraj = spikeArray[i][1];
        int spikeY = spikeArray[i][2];
        if (spikeArray[i][0] && spikeY >= 117 && spikeY <= 130 && relativeY<25 && abs(playerX+7-spiketraj) < 19) {
            collided = 1;
        }
    }
}

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
    if (((switchin & 2) == 2) && (relativeY==0) && phase == 1) {
        yVel = 10;
        Sound_Jump();
        relativeY = 1; // to get off ground
    }
    // 3) move sprites
    prevplayerX = playerX;
    prevrelY = relativeY;
    if (relativeY==0) {
        playerX = max(min(slidepot/48 + 14, prevplayerX+8), prevplayerX-8);
        // playerX = max(slidepot/48 + 14, prevplayerX-8);
    }
    if (relativeY > 0) {
        yVel--;
    } else {
        relativeY = 0; // fix to ground
        yVel = 0;
    }
    relativeY += yVel;
    relativeY = max(0,relativeY);

    // 4) start sounds

    // triggers at notes per second speed
    if (oneSecond%(30/NPS) == 0) {
        backgroundIndex++;
        backgroundIndex %= BACKGROUNDMUSICSIZE;
        TIMG8->COUNTERREGS.LOAD = backgroundMusic[backgroundIndex]-1; // set reload register
    }
    // 5) set semaphore
    flag = 1;

    // Collisions
    detectCollisions();

    // LEDs
    if (slowdown++%5 == 0 && phase==0) LED_Toggle(LEDindex++%3);
    oneSecond++;
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

/*
const uint8_t SinWave[32] = {
16,19,22,24,27,28,30,31,31,
31,30,28,27,24,22,19,16,13,10,
8,5,4,2,1,1,1,2,4,5,8,10,13};
*/
const uint8_t SinWave[32] = {
8,9,11,12,13,14,15,15,16,
15,15,14,13,12,11,9,8,6,5,
4,3,2,1,1,1,1,1,2,3,4,6,7};

int waveIndex = 0;
// my background music handler
void TIMG8_IRQHandler(void){
  if((TIMG8->CPU_INT.IIDX) == 1){ // this will acknowledge
    // GPIOB->DOUTTGL31_0 = (1<<27);   // toggle PB27
    DAC5_Out(SinWave[waveIndex++%32]);


  }
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
const char Language_English[]="< English > "; // make sure there are enough spaces to override
const char Language_Italian[]="< Italiano >";
const char Language_Klingon[]="< Klingon > ";
const char Start_English[]="Press Up to Start \n               "; // make sure there are enough spaces to override
const char Start_Italian[]="   Premi Su per   \n       Iniziare";
const char Start_Klingon[]="      yIvum       \n               ";
const char End_English[]=  " You Lose! Retry? "; // make sure there are enough spaces to override
const char End_Italian[]=  " Perdi! Riprovare?";
const char End_Klingon[]=  " qeylIS! nuqjatlh?";
const char Score_English[]="       Score:     "; // make sure there are enough spaces to override
const char Score_Italian[]="       Punto:     ";
const char Score_Klingon[]="      pe''eg:     ";
const char Highscore_English[]=  "    High Score:   "; // make sure there are enough spaces to override
const char Highscore_Italian[]=  "  Punteggio alto: ";
const char Highscore_Klingon[]=  "      Qapla':     ";
const char *Phrases[7][3]={
  {Hello_English,Hello_Italian,Hello_Klingon},
  {Goodbye_English,Goodbye_Italian,Goodbye_Klingon},
  {Language_English,Language_Italian,Language_Klingon},
  {Start_English,Start_Italian,Start_Klingon},
  {End_English,End_Italian,End_Klingon},
  {Score_English,Score_Italian,Score_Klingon},
  {Highscore_English,Highscore_Italian,Highscore_Klingon}
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
      Sound_Button(); // call one of your sounds
    }
    if((last == 0)&&(now == 2)){
      Sound_Jump(); // call one of your sounds
    }
    if((last == 0)&&(now == 4)){
      Sound_Explosion(); // call one of your sounds
    }
    if((last == 0)&&(now == 8)){
      Sound_Fastinvader1(); // call one of your sounds
    }
    last = now;
    // modify this to test all your sounds
  }
}

int langIndex = 0;
void drawLangStart() {
  ST7735_SetCursor(2,8);
  ST7735_OutString((char *)Phrases[3][langIndex]);
  ST7735_SetCursor(5,14);
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

void MusicOn() {
    TIMG8->COUNTERREGS.CTRCTL |= 0x01;
}
void MusicOff() {
    TIMG8->COUNTERREGS.CTRCTL &= (~0x01);
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
  TimerG8_IntArm(4000,1,2);
  // initialize all data structures
  int score = 0;
  __enable_irq();


  // TESTING
  /*
  ST7735_FillScreen(ST7735_YELLOW);
  SmartFill(5,5,80,80);
  */
  int highscore = 0;

  // the big overall loop
  while(1){

  // Start screen. Phase 0
  for (int i = 0 ; i < 3 ; i++) LED_Off(i);
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
      if ((switchin&8) == 8) {
          if ((TIMG8->COUNTERREGS.CTRCTL & 0x01) == 0) MusicOn();
          else MusicOff();
          while ((switchin&8) == 8) {}
      }
      // language change?
      switchLanguage();
  }

  // PHASE 1
  phase = 1;
  LED_Off(0); LED_Off(1); LED_Off(2);

  DrawStars();
  DrawRoad();
  spikeSpeed = 1.1;
  slowdown = 0;

  while(1){
    // wait for semaphore
      while (flag == 0) {}
       // clear semaphore
      flag = 0;
       // update ST7735R
      ST7735_DrawBitmap(playerX, 150 - relativeY, AlienMiddle, 15, 30);
      if (Random32() % (int)(42-5*spikeSpeed) == 0 && spikeCount<MAXSPIKES) {
          spawnSpike();
      }
      moveSpikes();


      // draw player : fine tuned to optimize refreshes
      if (true || prevplayerX != playerX) {
          int vel = abs(prevplayerX - playerX)+3;
          //ST7735_FillRect(12, 121, 104, 34, 0xFFFF); // problematic tbh
          if (playerX > prevplayerX) ST7735_FillRect(max(12,playerX-vel-spikeCount), 121-relativeY , vel+spikeCount, 34, ST7735_WHITE);
          if (playerX < prevplayerX) ST7735_FillRect(playerX+15, 121-relativeY, min(vel+spikeCount,99-playerX), 34, ST7735_WHITE);

          if (relativeY > 0 || prevrelY != 0) {
              if (yVel<=0) {
                  SmartFill(prevplayerX, 150 - prevrelY - 30 - 2 - spikeCount + 2*yVel, 15, 7-2*yVel + spikeCount);
                  // SmartFill(max(12,playerX-vel), 150 - prevrelY - 30 - 2 + 2*yVel, vel, 34);
              } else {
                  ST7735_FillRect(prevplayerX, 150 - relativeY - 2, 15, 6+3*yVel, ST7735_WHITE);
              }
          } else {
              if (prevrelY != 0) {
                  // SmartFill(prevplayerX-1, 150 - prevrelY - 30 - 9, 15+2, 14);
                  // cleanup(); // too slow
              }
              ST7735_Line(prevplayerX, 150-31, prevplayerX+15, 150-31, ST7735_WHITE);
          }

          if (relativeY > 0) {
              ST7735_DrawBitmap(playerX, 150 - prevrelY, AlienJump, 15, 30);
          } else {
              if (slowdown%(4*5)<5) {
                  ST7735_DrawBitmap(playerX, 150 - prevrelY, AlienLeft, 15, 30);
              } else if (slowdown%(4*5)<10) {
                  ST7735_DrawBitmap(playerX, 150 - prevrelY, AlienMiddle, 15, 30);
              } else if (slowdown%(4*5)<15) {
                  ST7735_DrawBitmap(playerX, 150 - prevrelY, AlienRight, 15, 30);
              } else {
                  ST7735_DrawBitmap(playerX, 150 - prevrelY, AlienMiddle, 15, 30);
              }
          }
      }

      // increase game difficulty
      if (slowdown%120 == 0 && spikeSpeed<5.4) {
          spikeSpeed += 0.25; // linear growth with cap of 5.4
      }

      // draw score
      score += (slowdown%5==0)?10:0;
      ST7735_SetCursor(0,0);
      drawScore(score);

    // check for end game or level switch
      if (collided) {
          break;
      }
      slowdown++;
  }

  // PHASE 2
  // reset game data
  spikeSpeed = 1.1;
  LEDindex = 0;
  phase = 2;
  collided = 0;
  for (int i = 0 ; i < MAXSPIKES ; i++) {
      for (int j = 0 ; j< 3 ; j++) spikeArray[i][j] = 0;
  }
  spikeCount = 0;
  highscore = max(score, highscore);

  // graphics
  ST7735_FillScreen(ST7735_BLACK);
  DrawStars();
  DrawSpike(64, 10, 2);
  // game over text
    ST7735_SetCursor(2,6);
    ST7735_OutString((char *)Phrases[4][langIndex]);
    ST7735_SetCursor(2,10);
    ST7735_OutString((char *)Phrases[5][langIndex]);
    ST7735_SetCursor(9,11);
    drawScore(score);
    ST7735_SetCursor(2,13);
    ST7735_OutString((char *)Phrases[6][langIndex]);
    ST7735_SetCursor(9,14);
    drawScore(highscore);
    score = 0;

  while (1) {
      // wait for retry
      while (!flag) {}
      flag = 0;
      if (slowdown%15 == 0) {
          for (int i = 0 ; i < 3 ; i++) LED_Toggle(i);
      }
      slowdown++;
      if ((switchin&2)==2) break;
  }
  while ((switchin&2)==2){}; // wait for switch release
  ST7735_FillScreen(ST7735_BLACK);
  }
}
