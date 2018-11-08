#include "TTGO-RGB-Pixel.h"

extern EventGroupHandle_t eventHandle;
Adafruit_NeoPixel *strip = NULL;

bool _set_colour(uint16_t num, uint32_t colour)
{
    bool ret = false;
    if (xEventGroupWaitBits(eventHandle, RGB_RUN, pdFALSE, pdTRUE, 100 / portTICK_PERIOD_MS) & RGB_RUN)
    {
        strip->setPixelColor(num, colour);
        strip->show();
    }
    else
    {
        ret = true;
    }
    return ret;
}

void TTGO_RGB_setBrightness(uint8_t level)
{
    strip->setBrightness(level);
    strip->show();
}

void TTGO_RGB_setState(uint8_t state)
{
    switch (state)
    {
    case 0:
        for (uint16_t i = 0; i < strip->numPixels(); i++)
        {
            strip->setPixelColor(i, strip->Color(0, 0, 0));
            strip->show();
        }
        break;
    case 1:
        for (uint16_t i = 0; i < strip->numPixels(); i++)
        {
            strip->setPixelColor(i, strip->Color(255, 25, 127));
            strip->show();
        }
        break;
    default:
        break;
    }
}

void TTGO_RGB_init(uint8_t pin,uint16_t nums)
{
    strip = new Adafruit_NeoPixel(nums, pin, NEO_GRB + NEO_KHZ800);
    strip->setBrightness(100);
    strip->begin();
    strip->show();
}

void TTGO_RGB_setNums(uint16_t nums)
{
    strip->updateLength(nums);
}

uint32_t Wheel(byte WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return strip->Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return strip->Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Slightly different, this makes the rainbow equally distributed throughout
void TTGO_RGB_rainbowCycle(uint8_t wait)
{
    EventBits_t bit;
    uint16_t i, j;

    for (j = 0; j < 256 * 5; j++)
    {
        for (i = 0; i < strip->numPixels(); i++)
        {
            if (_set_colour(i, Wheel(((i * 256 / strip->numPixels()) + j) & 255)))
                return;
        }
        strip->show();
        delay(wait);
    }
}

//Theatre-style crawling lights with rainbow effect
void TTGO_RGB_theaterChaseRainbow(uint8_t wait)
{
    for (int j = 0; j < 256; j++)
    {
        for (int q = 0; q < 3; q++)
        {
            for (uint16_t i = 0; i < strip->numPixels(); i = i + 3)
            {
                if (_set_colour(i + q, Wheel((i + j) % 255)))
                    return;
            }
            strip->show();

            delay(wait);

            for (uint16_t i = 0; i < strip->numPixels(); i = i + 3)
            {
                if (_set_colour(i + q, 0))
                    return;
            }
        }
    }
}

void TTGO_RGB_rainbow(uint8_t wait)
{
    uint16_t i, j;

    for (j = 0; j < 256; j++)
    {
        for (i = 0; i < strip->numPixels(); i++)
        {
            if (_set_colour(i, Wheel((i + j) & 255)))
                return;
        }
        strip->show();
        delay(wait);
    }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait)
{
    for (uint16_t i = 0; i < strip->numPixels(); i++)
    {
        strip->setPixelColor(i, c);
        strip->show();
        delay(wait);
    }
}

void TTGO_RGB_breathe(void)
{
    for (uint8_t j = 0; j < 256; j++)
    {
        for (uint16_t i = 0; i < strip->numPixels(); i++)
        {
            if (_set_colour(i, strip->Color(0, 0, 0, strip->gamma8(j))))
                return;
        }
        strip->show();
    }
    delay(2000);
    for (uint8_t j = 255; j >= 0; j--)
    {
        for (uint16_t i = 0; i < strip->numPixels(); i++)
        {
            if (_set_colour(i, strip->Color(0, 0, 0, strip->gamma8(j))))
                return;
        }
        strip->show();
    }
    delay(2000);
}

uint8_t red(uint32_t c)
{
    return (c >> 16);
}
uint8_t green(uint32_t c)
{
    return (c >> 8);
}
uint8_t blue(uint32_t c)
{
    return (c);
}

void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops)
{
    float fadeMax = 100.0;
    int fadeVal = 0;
    uint32_t wheelVal;
    int redVal, greenVal, blueVal;

    for (int k = 0; k < rainbowLoops; k++)
    {

        for (int j = 0; j < 256; j++)
        { // 5 cycles of all colors on wheel

            for (int i = 0; i < strip->numPixels(); i++)
            {

                wheelVal = Wheel(((i * 256 / strip->numPixels()) + j) & 255);

                redVal = red(wheelVal) * float(fadeVal / fadeMax);
                greenVal = green(wheelVal) * float(fadeVal / fadeMax);
                blueVal = blue(wheelVal) * float(fadeVal / fadeMax);

                if (_set_colour(i, strip->Color(redVal, greenVal, blueVal)))
                    return;
            }

            //First loop, fade in!
            if (k == 0 && fadeVal < fadeMax - 1)
            {
                fadeVal++;
            }

            //Last loop, fade out!
            else if (k == rainbowLoops - 1 && j > 255 - fadeMax)
            {
                fadeVal--;
            }

            strip->show();
            delay(wait);
        }
    }

    delay(500);

    for (int k = 0; k < whiteLoops; k++)
    {

        for (int j = 0; j < 256; j++)
        {

            for (uint16_t i = 0; i < strip->numPixels(); i++)
            {
                if (_set_colour(i, strip->Color(0, 0, 0, strip->gamma8(j))))
                    return;
            }
            strip->show();
        }

        delay(2000);
        for (int j = 255; j >= 0; j--)
        {

            for (uint16_t i = 0; i < strip->numPixels(); i++)
            {
                if (_set_colour(i, strip->Color(0, 0, 0, strip->gamma8(j))))
                    return;
            }
            strip->show();
        }
    }

    delay(500);
}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength)
{

    if (whiteLength >= strip->numPixels())
        whiteLength = strip->numPixels() - 1;

    int head = whiteLength - 1;
    int tail = 0;

    int loops = 3;
    int loopNum = 0;

    static unsigned long lastTime = 0;

    while (true)
    {
        for (int j = 0; j < 256; j++)
        {
            for (uint16_t i = 0; i < strip->numPixels(); i++)
            {
                if ((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head))
                {
                    if (_set_colour(i, strip->Color(0, 0, 0, 255)))
                        return;
                }
                else
                {
                    if (_set_colour(i, Wheel(((i * 256 / strip->numPixels()) + j) & 255)))
                        return;
                }
            }

            if (millis() - lastTime > whiteSpeed)
            {
                head++;
                tail++;
                if (head == strip->numPixels())
                {
                    loopNum++;
                }
                lastTime = millis();
            }

            if (loopNum == loops)
                return;

            head %= strip->numPixels();
            tail %= strip->numPixels();
            strip->show();
            delay(wait);
        }
    }
}