#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include <ArduinoJson.h>
#include <FastLED.h>

#include "config.h"
#include "enum.h"
#include "mapping.h"
#include "pins.h"
#include "webserver.h"

const char* SSID = "VM5105076";
const char* PASSWORD = "c4wvPhdwmsmp";
const IPAddress ip(192,168,0,10);
const IPAddress gateway(192,168,0,1);
const IPAddress subnet(255,255,255,0);

bool updateInProgress = false;

CRGB leds[NUM_LEDS];


ESP8266WebServer server(80);


APIRequestHandler apihandler;
FileRequestHandler filehandler;
UploadRequestHandler uploadHandler;

bool on = true;

enum class LightMode
{
  STATIC,
  TWINKLE,
  CHASER,
};
ENUM_SERIALISATION(LightMode, {
  {LightMode::STATIC, "static"},
  {LightMode::TWINKLE, "twinkle"},
  {LightMode::CHASER, "chaser"}
});

LightMode lightMode;


enum class ColourMode
{
  STATIC,
  RAINBOW,
  NOISE_HUE
};
ENUM_SERIALISATION(ColourMode, {
  {ColourMode::STATIC, "static"},
  {ColourMode::RAINBOW, "rainbow"},
  {ColourMode::NOISE_HUE, "noise-hue"}
});

ColourMode colourMode;
uint32_t staticColour;

void setup()
{
  lightMode = LightMode::STATIC;
  colourMode = ColourMode::STATIC;
  staticColour = 0xffffff;

  FastLED.addLeds<WS2811, PIN_LEDS, GRB>(leds, NUM_LEDS).setCorrection(LED_CORRECTION);
  FastLED.setBrightness(15);
  FastLED.show();

  leds[0] = 0x00FF00;
  leds[27] = 0x00FF00;
  leds[28] = 0x00FF00;
  FastLED.show();

  if (!LittleFS.begin())
  {
    leds[0] = 0xFF0000;
    leds[27] = 0xFF0000;
    leds[28] = 0xFF0000;
    FastLED.show();

    while (true);
  }

  // setup OTA callback
  ArduinoOTA.onStart([]() {
    updateInProgress = true;
    if (ArduinoOTA.getCommand() == U_FS)
      LittleFS.end();
  });

  ArduinoOTA.onEnd([]() {
    if (ArduinoOTA.getCommand() == U_FS)
      LittleFS.begin();
    fill_solid(leds, NUM_LEDS, 0xffffff);
    FastLED.show();
    updateInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    const int8_t led_progress = (progress / static_cast<double>(total)) * 28;
    for (size_t i = 0; i < NUM_LEDS; ++i)
      leds[i] = (27 - led_height(i)) < led_progress ? 0x00FF00 : 0x000000;
    FastLED.show();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR)
      leds[0] = 0xFF0000;
    else if (error == OTA_BEGIN_ERROR)
      leds[1] = 0xFF0000;
    else if (error == OTA_CONNECT_ERROR)
      leds[2] = 0xFF0000;
    else if (error == OTA_RECEIVE_ERROR)
      leds[3] = 0xFF0000;
    else if (error == OTA_END_ERROR)
      leds[4] = 0xFF0000;
    FastLED.show();
  });

  // setup WiFi
  WiFi.begin(SSID, PASSWORD);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  leds[1] = 0x0000FF;
  leds[26] = 0x0000FF;
  leds[29] = 0x0000FF;
  FastLED.show();

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }
  leds[1] = 0x00FF00;
  leds[26] = 0x00FF00;
  leds[29] = 0x00FF00;
  FastLED.show();

  // setup OTA
  ArduinoOTA.setHostname("dodge-cube");
  ArduinoOTA.begin();

  // setup webserver
  apihandler.addRoute("status", [](ESP8266WebServer& server) {
    JsonDocument document;
    document["on"] = on;
    document["mode"] = serialise_enum(lightMode);
    document["colour"] = serialise_enum(colourMode);

    server.setContentLength(measureJson(document));
    server.send(200, "application/json", "");
    serializeJson(document, server.client());
  });
  apihandler.addRoute("off", [](ESP8266WebServer& server) {
    on = false;
    fill_solid(leds, NUM_LEDS, 0x000000);
    FastLED.show();
    server.send(200);
  });
  apihandler.addRoute("on", [](ESP8266WebServer& server) {
    on = true;
    server.send(200);
  });
  apihandler.addRoute("set-mode", [](ESP8266WebServer& server) {
    lightMode = deserialise_enum<LightMode>(server.arg("mode").c_str());
    server.send(200);
  });
  apihandler.addRoute("set-colour-mode", [](ESP8266WebServer& server) {
    colourMode = deserialise_enum<ColourMode>(server.arg("mode").c_str());
    server.send(200);
  });
  apihandler.addRoute("set-static-colour", [](ESP8266WebServer& server) {
    colourMode = ColourMode::STATIC;
    staticColour = server.arg("colour").toInt();
    server.send(200);
  });
  server.enableCORS(true);
  server.addHandler(&apihandler);
  server.addHandler(&filehandler);
  server.addHandler(&uploadHandler);
  server.begin();
}


CRGB getColour()
{
  switch (colourMode)
  {
    case ColourMode::STATIC:
      return staticColour;
    case ColourMode::NOISE_HUE:
      return CHSV(inoise8(millis() / 20), 0xFF, 0xFF);
    case ColourMode::RAINBOW:
      return CHSV(millis() / 32, 0xFF, 0xFF);
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
      leds[i] = 0xFF0000;
    else if (this_height <= height)
      leds[i] = CHSV(0xFF, (0x7F / TIP_LENGTH) * (TIP_LENGTH - (height - this_height)), 0x00);
    else
      leds[i] = 0x000000;
  }
}


void twinkle()
{
  for (uint8_t i = 0; i < NUM_LEDS; ++i)
    leds[i] = leds[i].nscale8(255 - 4);

  if (random(2) == 0)
    leds[random(NUM_LEDS)] = getColour();
}


// an even edge always points towards a 4-edge vertex
// an odd edge always points away from a 4-edge vertex
// ditto/vice-versa for 3-edge verties (even points away, odd points towards)
uint8_t graph[][24][3] = {
  {
    {4, 3, 0},    //  0
    {12, 13, 0},  //  1
    {17, 1, 0},   //  2
    {20, 21, 2},  //  3
    {3, 0, 0},    //  4
    {4, 11, 10},  //  5
    {10, 9, 0},   //  6
    {18, 19, 9},  //  7
    {23, 7, 0},   //  8
    {14, 15, 8},  //  9
    {9, 6, 0},    // 10
    {10, 5, 4},   // 11
    {16, 15, 0},  // 12
    {0, 1, 12},   // 13
    {11, 13, 0},  // 14
    {8, 9, 14},   // 15
    {15, 12, 0},  // 16
    {22, 23, 16}, // 17
    {22, 21, 0},  // 18
    {6, 7, 18},   // 19
    {5, 19, 0},   // 20
    {2, 3, 20},   // 21
    {21, 18, 0},  // 22
    {16, 17, 22}  // 23
  },
  {
    {1, 12, 13},  //  0
    {2, 17, 0},   //  1
    {3, 20, 21},  //  2
    {0, 4, 0},    //  3
    {11, 10, 9},  //  4
    {19, 20, 0},  //  5
    {7, 18, 19},  //  6
    {8, 23, 0},   //  7
    {9, 14, 15},  //  8
    {6, 10, 0},   //  9
    {5, 4, 11},   // 10
    {13, 14, 0},  // 11
    {13, 0, 1},   // 12
    {14, 11, 0},  // 13
    {15, 8, 9},   // 14
    {12, 16, 0},  // 15
    {17, 22, 23}, // 16
    {1, 2, 0},    // 17
    {19, 6, 7},   // 18
    {20, 5, 0},   // 19
    {21, 2, 3},   // 20
    {18, 22, 0},  // 21
    {23, 16, 17}, // 22
    {7, 8, 0}     // 23
  }
};

void chaser()
{
  const uint32_t speed = 50;
  constexpr uint8_t decay = 255 - 2;

  struct Particle
  {
    CRGB colour;
    uint32_t speed;
    uint32_t last;
    uint8_t decay;
    uint8_t position;
    bool forwards;
  };
  static Particle particles[] = {
    {CRGB(0xFF0000), 50+10, 0, 255 - 2, 0, true},
    {CRGB(0x00FF00), 50+20, 0, 255 - 2, 0, true},
    {CRGB(0x0000FF), 50+30, 0, 255 - 2, 0, true},
    {CRGB(0xFF0000), 50+60, 0, 255 - 2, 0, true},
    {CRGB(0x00FF00), 50+50, 0, 255 - 2, 0, true},
    {CRGB(0x0000FF), 50+40, 0, 255 - 2, 0, true}
  };

  const uint32_t now = millis();

  for (uint8_t i = 0; i < NUM_LEDS; ++i)
    leds[i] = leds[i].nscale8(decay);

  for (auto& particle : particles)
  {
    if (now - particle.last <= particle.speed)
      continue;
    particle.last = now;

    const uint8_t segment = particle.position / 7;
    uint8_t next = particle.position + (particle.forwards * 2 - 1);

    if (segment != next / 7)
    {
      next = graph[particle.forwards][segment][random(((segment + particle.forwards) & 1) + 2)];
      particle.forwards = particle.forwards != ((segment & 1) == (next & 1));
      next = next * 7 + (6 * !particle.forwards);
    }
    particle.position = next;
    leds[particle.position] |= particle.colour;
  }
}


void upwave()
{
  const uint8_t height = (inoise8(millis() / 5) / 255.0) * 28;
  const uint8_t TIP_LENGTH = 3;

  for (uint8_t i = 0; i < NUM_LEDS; ++i)
  {
    const uint8_t this_height = led_height(i);
    const uint16_t hue = inoise8(millis() / 20, this_height * 16);
    if (this_height <= height - TIP_LENGTH)
      leds[i] = CHSV(hue, 0xFF, 0xFF);
    else if (this_height <= height)
      leds[i] = CHSV(hue, (0xFF / TIP_LENGTH) * ((height - this_height)), 0xFF);
    else
      leds[i] = 0x000000;
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
      fill_solid(leds, NUM_LEDS, getColour());
    break;
    case LightMode::TWINKLE:
      twinkle();
    break;
    case LightMode::CHASER:
      chaser();
    break;
  }

  FastLED.show();
}
