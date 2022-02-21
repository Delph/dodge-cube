#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include <FastLED.h> // for math
#include <Adafruit_NeoPixel.h> // for leds

#include "config.h"
#include "mapping.h"
#include "pins.h"
#include "webserver.h"

const char* SSID = "";
const char* PASSWORD = "";
const IPAddress ip(192,168,0,10);
const IPAddress gateway(192,168,0,1);
const IPAddress subnet(255,255,255,0);

bool updateInProgress = false;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);


APIRequestHandler apihandler;
FileRequestHandler filehandler;
UploadRequestHandler uploadhandler;

bool on = true;

enum class LightMode
{
  STATIC,

  FLAME,
  TWINKLE,
  UP_WAVE,
};
LightMode lm_from_string(const String& str)
{
  if (str == "static")
    return LightMode::STATIC;
  if (str == "flame")
    return LightMode::FLAME;
  if (str == "twinkle")
    return LightMode::TWINKLE;
  if (str == "up-wave")
    return LightMode::UP_WAVE;

  return LightMode::STATIC;
}
LightMode lightMode;


enum class ColourMode
{
  STATIC,

  RAINBOW,
  NOISE_HUE
};
ColourMode cm_from_string(const String& str)
{
  if (str == "static")
    return ColourMode::STATIC;
  if (str == "rainbow")
    return ColourMode::RAINBOW;
  if (str == "noise-hue")
    return ColourMode::NOISE_HUE;

  return ColourMode::STATIC;
}
ColourMode colourMode;
uint32_t staticColour;

void setup()
{
  lightMode = LightMode::STATIC;
  colourMode = ColourMode::STATIC;
  staticColour = 0xffffff;

  // start LEDs
  pixels.begin();

  pixels.setPixelColor(0, 0x00FF00);
  pixels.setPixelColor(27, 0x00FF00);
  pixels.setPixelColor(28, 0x00FF00);
  pixels.show();

  if (!LittleFS.begin())
  {
    pixels.setPixelColor(0, 0xFF0000);
    pixels.setPixelColor(27, 0xFF0000);
    pixels.setPixelColor(28, 0xFF0000);
    pixels.show();

    while (true);
  }

  // setup OTA callback
  ArduinoOTA.onStart([]() {
    updateInProgress = true;
    if (ArduinoOTA.getCommand() == U_FS)
      LittleFS.end();
  });

  ArduinoOTA.onEnd([]() {
    pixels.fill(0xffffff);
    pixels.show();
    updateInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    const int8_t led_progress = (progress / static_cast<double>(total)) * 28;
    for (size_t i = 0; i < NUM_LEDS; ++i)
      pixels.setPixelColor(i, (27 - led_height(i)) < led_progress ? 0x00FF00 : 0x000000);
    pixels.show();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR)
      pixels.setPixelColor(0, 0xFF0000);
    else if (error == OTA_BEGIN_ERROR)
      pixels.setPixelColor(1, 0xFF0000);
    else if (error == OTA_CONNECT_ERROR)
      pixels.setPixelColor(2, 0xFF0000);
    else if (error == OTA_RECEIVE_ERROR)
      pixels.setPixelColor(3, 0xFF0000);
    else if (error == OTA_END_ERROR)
      pixels.setPixelColor(4, 0xFF0000);
  });

  // setup WiFi
  WiFi.begin(SSID, PASSWORD);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  pixels.setPixelColor(1, 0x0000FF);
  pixels.setPixelColor(26, 0x0000FF);
  pixels.setPixelColor(29, 0x0000FF);
  pixels.show();

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }
  pixels.setPixelColor(1, 0x00FF00);
  pixels.setPixelColor(26, 0x00FF00);
  pixels.setPixelColor(29, 0x00FF00);
  pixels.show();

  // setup OTA
  ArduinoOTA.setPort(2000);
  ArduinoOTA.setHostname("dodge-cube");
  ArduinoOTA.begin();

  // setup webserver
  apihandler.addRoute("off", [](ESP8266WebServer& server) {
    on = false;
    pixels.fill(0x000000);
    pixels.show();
    server.send(200);
  });
  apihandler.addRoute("on", [](ESP8266WebServer& server) {
    on = true;
    server.send(200);
  });
  apihandler.addRoute("set-mode", [](ESP8266WebServer& server) {
    lightMode = lm_from_string(server.arg("mode"));
    server.send(200);
  });
  apihandler.addRoute("set-colour-mode", [](ESP8266WebServer& server) {
    colourMode = cm_from_string(server.arg("mode"));
    server.send(200);
  });
  apihandler.addRoute("set-static-colour", [](ESP8266WebServer& server) {
    staticColour = server.arg("colour").toInt();
    server.send(200);
  });
  server.addHandler(&apihandler);
  server.addHandler(&filehandler);
  server.addHandler(&uploadhandler);
  server.begin();
}


uint32_t getColour()
{
  switch (colourMode)
  {
    case ColourMode::STATIC:
      return staticColour;
    case ColourMode::NOISE_HUE:
      return pixels.ColorHSV(inoise8(millis() / 20) << 8, 0xFF, 0xFF);
    case ColourMode::RAINBOW:
      return pixels.ColorHSV(millis() << 8, 0xFF, 0xFF);
  }
  return 0xffffff;
}


void flame()
{
  const uint8_t height = (inoise8(millis() / 5) / 255.0) * 28;
  // const int8_t tilt = 0;
  // const uint8_t angle = 0;
  const uint8_t TIP_LENGTH = 3;

  for (uint8_t i = 0; i < NUM_LEDS; ++i)
  {
    const uint8_t this_height = led_height(i);
    if (this_height <= height - TIP_LENGTH)
      pixels.setPixelColor(i, 0xFF0000);
    else if (this_height <= height)
      pixels.setPixelColor(i, 0xFF, (0x7F / TIP_LENGTH) * (TIP_LENGTH - (height - this_height)), 0x00);
    else
      pixels.setPixelColor(i, 0x000000);
  }
}


void twinkle()
{
  for (uint8_t i = 0; i < NUM_LEDS; ++i)
  {
    const CRGB colour = (CRGB(pixels.getPixelColor(i))).nscale8(255 - 4);
    pixels.setPixelColor(i, colour.r, colour.g, colour.b);
  }

  if (random(2) == 0)
    pixels.setPixelColor(random(NUM_LEDS), getColour());
}


void upwave()
{
  const uint8_t height = (inoise8(millis() / 5) / 255.0) * 28;
  const uint8_t TIP_LENGTH = 3;

  for (uint8_t i = 0; i < NUM_LEDS; ++i)
  {
    const uint8_t this_height = led_height(i);
    const uint16_t hue = inoise8(millis() / 20, this_height * 16) << 8;
    if (this_height <= height - TIP_LENGTH)
      pixels.setPixelColor(i, pixels.ColorHSV(hue, 0xFF, 0xFF));
    else if (this_height <= height)
      pixels.setPixelColor(i, pixels.ColorHSV(hue, (0xFF / TIP_LENGTH) * ((height - this_height)), 0xFF));
    else
      pixels.setPixelColor(i, 0x000000);
  }
}


void loop()
{
  // handle any OTA stuff
  ArduinoOTA.handle();

  // if we're updating, don't do anything else
  if (updateInProgress)
    return;

  // handle webserver stuff
  server.handleClient();

  if (!on)
    return;

  switch (lightMode)
  {
    case LightMode::STATIC:
      pixels.fill(getColour());
    break;
    case LightMode::FLAME:
      flame();
    break;
    case LightMode::TWINKLE:
      twinkle();
    break;
    case LightMode::UP_WAVE:
      upwave();
    break;
  }

  pixels.show();
}
