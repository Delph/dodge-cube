#include <Arduino.h>

#include <FastLED.h>

#include "config.h"
#include "pins.h"

CRGB leds[NUM_LEDS];


void setup()
{
  FastLED.addLeds<WS2811, PIN_LEDS, GRB>(leds, NUM_LEDS).setCorrection(LED_CORRECTION);
  FastLED.setBrightness(15);
  FastLED.show();
}

void loop()
{
  fill_solid(leds, NUM_LEDS, 0x3F0000);

  FastLED.show();
}
