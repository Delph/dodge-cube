#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include <FastLED.h> // for math
#include <Adafruit_NeoPixel.h> // for leds

#include "config.h"
#include "mapping.h"
#include "pins.h"

const char* SSID = "";
const char* PASSWORD = "";
const IPAddress ip(192,168,0,10);
const IPAddress gateway(192,168,0,1);
const IPAddress subnet(255,255,255,0);

bool updateInProgress = false;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);

void serve(ESP8266WebServer& server, const char* const path)
{
  File file = LittleFS.open(path, "r");
  if (!file)
  {
    server.send(404, "text/html", path);
    return;
  }

  char buf[file.size() + 1];
  file.readBytes(buf, file.size());
  buf[file.size()] = 0;
  file.close();

  server.send(200, mime::getContentType(path), buf);
}

class UploadRequestHandler : public RequestHandler
{
  bool canHandle(HTTPMethod method, const String& uri) override
  {
    return method == HTTP_POST && uri == "/do-upload.html";
  }
  bool canUpload(const String& uri) override
  {
    return uri == "/do-upload.html";
  }

  void upload(ESP8266WebServer& server, const String& uri, HTTPUpload& upload) override
  {
    if (!canUpload(uri))
      return;

    static File file;

    switch (upload.status)
    {
      case UPLOAD_FILE_START:
        if (file)
          file.close();

        file = LittleFS.open(upload.filename.c_str(), "w");
      break;
      case UPLOAD_FILE_WRITE:
        if (file)
          file.write(upload.buf, upload.currentSize);
      break;
      case UPLOAD_FILE_END:
        if (file)
        {
          file.close();
          serve(server, "/success.html");
        }
      break;
      default:
        if (file)
          file.close();
        server.send(500, "text/plain", "Upload failed.");
      break;
    }
  }
};
UploadRequestHandler uploadhandler;


class FileRequestHandler : public RequestHandler
{
  bool canHandle(HTTPMethod method, const String& uri) override
  {
    return method == HTTP_GET;
  }

  bool handle(ESP8266WebServer& server, HTTPMethod method, const String& uri) override
  {
    if (!canHandle(method, uri))
      return false;

    const char* path = uri == "/" ? "/index.html" : uri.c_str();
    serve(server, path);
    return true;
  }
};
FileRequestHandler filehandler;


class APIRequestHandler : public RequestHandler
{
  bool canHandle(HTTPMethod method, const String& uri) override
  {
    return uri.startsWith("/api");
  }

  bool handle(ESP8266WebServer& server, HTTPMethod method, const String& uri) override
  {
    server.send(200, "text/html", "OK");
    return true;
  }
};
APIRequestHandler apihandler;


void setup()
{
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
    const size_t led_progress = (progress / static_cast<double>(total)) * 28;
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
  server.addHandler(&uploadhandler);
  server.addHandler(&filehandler);
  server.begin();
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

void flame()
{
  const uint8_t height = (inoise8(millis() / 5) / 255.0) * 28;
  const int8_t tilt = 0;
  const uint8_t angle = 0;
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



void loop()
{
  // handle any OTA stuff
  ArduinoOTA.handle();

  // if we're updating, don't do anything else
  if (updateInProgress)
    return;

  // handle webserver stuff
  server.handleClient();

  upwave();
  // upstep();

  pixels.show();
}
