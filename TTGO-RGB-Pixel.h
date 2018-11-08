#pragma once
#include <Adafruit_NeoPixel.h>
#include "FreeRTOS.h"

enum
{
    RGB_RUN = 1 << 0,
    RGB_STOP = 1 << 1,
};

void TTGO_RGB_init(uint8_t pin,uint16_t nums);
void TTGO_RGB_setNums(uint16_t nums);
void TTGO_RGB_setState(uint8_t state);
void TTGO_RGB_setBrightness(uint8_t level);
void TTGO_RGB_breathe(void);
void TTGO_RGB_rainbow(uint8_t wait);
void TTGO_RGB_rainbowCycle(uint8_t wait);
void TTGO_RGB_theaterChaseRainbow(uint8_t wait);
void TTGO_RGB_theaterChase(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops);
void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength);