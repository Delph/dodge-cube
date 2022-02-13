#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <FastLED.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "mapping.h"
#include "pins.h"

const char* SSID = "";
const char* PASSWORD = "";
const IPAddress ip(192,168,0,10);
const IPAddress gateway(192,168,0,1);
const IPAddress subnet(255,255,255,0);


CRGB leds[NUM_LEDS];
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);


void setup()
{
  // setup OTA handling
  ArduinoOTA.onStart([]() {
    if (ArduinoOTA.getCommand() == U_FS)
    {
      // unmount filesystem using ES.end()
    }
  });

  ArduinoOTA.onEnd([]() {
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    const size_t led_progress = (progress / static_cast<double>(total)) * 28;
    for (size_t i = 0; i < NUM_LEDS; ++i)
      pixels.setPixelColor(i, led_height(i) <= led_progress ? pixels.Color(0x00, 0xFF, 0x00) : pixels.Color(0x0, 0x0, 0x0));
    pixels.show();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR)
    {
    }
    else if (error == OTA_BEGIN_ERROR)
    {
    }
    else if (error == OTA_CONNECT_ERROR)
    {
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
    }
    else if (error == OTA_END_ERROR)
    {
    }
  });

  // setup WiFi
  WiFi.begin(SSID, PASSWORD);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }

  // setup OTA
  ArduinoOTA.setPort(2000);
  ArduinoOTA.setHostname("dodge-cube");
  ArduinoOTA.begin();

  // start LEDs
  pixels.begin();
}


void upwave()
{
  const uint8_t height = (inoise8(millis() / 5) / 255.0) * 28;
  const uint8_t TIP_LENGTH = 3;

  fill_solid(leds, NUM_LEDS, 0x000000);
  for (uint8_t i = 0; i < NUM_LEDS; ++i)
  {
    const uint8_t this_height = led_height(i);
    const uint8_t hue = inoise8(millis() / 20, this_height * 16);
    if (this_height <= height - TIP_LENGTH)
      leds[i] = CHSV(hue, 0xFF, 0xFF);
    else if (this_height <= height)
      leds[i] = CHSV(hue, (0xFF / TIP_LENGTH) * ((height - this_height)), 0xFF);
  }
}

void flame()
{
  const uint8_t hue = millis() / 200;
  const uint8_t height = (inoise8(millis() / 5) / 255.0) * 28;
  const int8_t tilt = 0;
  const uint8_t angle = 0;
  const uint8_t TIP_LENGTH = 3;

  fill_solid(leds, NUM_LEDS, 0x000000);
  for (uint8_t i = 0; i < NUM_LEDS; ++i)
  {
    const uint8_t this_height = led_height(i);
    if (this_height <= height - TIP_LENGTH)
      leds[i] = 0xFF0000;
    else if (this_height <= height)
      leds[i] = CRGB(0xFF, (0x7F / TIP_LENGTH) * (TIP_LENGTH - (height - this_height)), 0x00);
  }
}



void loop()
{
  // handle any OTA stuff
  ArduinoOTA.handle();

  upwave();
  // upstep();

  for (size_t i = 0; i < NUM_LEDS; ++i)
    pixels.setPixelColor(i, pixels.Color(leds[i].r, leds[i].g, leds[i].b));

  pixels.show();
}
