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
#include "images/images.h"

// Helper functions from Lab9Main.cpp //
extern uint32_t M;
uint32_t Random32(void);
uint32_t Random(uint32_t n);
int max(int a, int b);
int min(int a, int b);
extern int phase;
extern double spikeSpeed;


// cleans up the edges of the screen after a jump
void cleanup() {
    int x = 8, y=75, w=25, h=75;
    for (int i = 0 ; i < w ; i++) {
        for (int j = 0 ; j < h ; j++) {
            // color logic
            if ((x+i)<64+(y+1+j)*0.42666 && (x+i)>64-(y+1+j)*0.42666) { // if is within road, white; else, black
                // ST7735_DrawPixel(x+i, y+j, ST7735_WHITE);
                break;
            } else {
                ST7735_DrawPixel(x+i, y+j, ST7735_BLACK);
            }
        }
    }
    x = 95; y=75; w=25; h=75;
    for (int i = w-1 ; i >=0 ; i--) {
        for (int j = 0 ; j < h ; j++) {
            // color logic
            if ((x+i)<64+(y+1+j)*0.42666 && (x+i)>64-(y+1+j)*0.42666) { // if is within road, white; else, black
                // ST7735_DrawPixel(x+i, y+j, ST7735_WHITE);
                break;
            } else {
                ST7735_DrawPixel(x+i, y+j, ST7735_BLACK);
            }
        }
    }
}


// similar to ST7735_FillRect, but instead of filling a solid color,
// this intelligently refills the background to match the road/space background color
void SmartFill(int x, int y, int w, int h) {
    for (int i = 0 ; i < w ; i++) {
        for (int j = 0 ; j < h ; j++) {
            // color logic
            if ((x+i)<64+(y+1+j)*0.42666 && (x+i)>64-(y+1+j)*0.42666) { // if is within road, white; else, black
                ST7735_DrawPixel(x+i, y+j, ST7735_WHITE);
            } else {
                ST7735_DrawPixel(x+i, y+j, ST7735_BLACK);
            }
        }
    }
}

// randomly draws stars
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

// draws the road
void DrawRoad() {
    for (int i = 0 ; i < 128 ; i++) {
        ST7735_Line(64, 0, i, 150, 0xFFFF);
        ST7735_Line(i,150,i,160,0xFFFF);
    }
}

// draws spikes
// known to work
// color 0 = red, 1 = white, 2 = endscreen
void DrawSpike(int x, int y, int color) {
    int height = y/6 + 1; // divisor is arbitrary. controls size of spike.
    if (color==2) { // special lose screen
        height = 120;
    }
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
            } else if (color==2) {
                // calculate vector from pixel to spike origin
                double vectX = (j-x), vectY = (y-i);
                double compX = 0.447, compY = -0.8944; // right edge slope = -2
                // normalize vectors
                double vectMag = sqrt(vectX*vectX + vectY*vectY);
                vectX /= vectMag;
                vectY /= vectMag;
                // normalized dot product with right edge, [0,1]
                double dotProd = vectX*compX + vectY*compY;
                // int angle = 3.141592/2 - dotProd; // arccos in radians,
                // set brightness
                int brightness = dotProd*165;
                ST7735_DrawPixel(j, i, ST7735_Color565(172+brightness/2, brightness, brightness));
            } else {
                if (spikeSpeed<1.6 && i-y > 7 && abs(j-x) < (i-y-7)/2) continue; // an attempt to refresh faster
                ST7735_DrawPixel(j, i, ST7735_WHITE);
            }

        }
    }

}
