#ifndef WEBSERVER_H_INCLUDE
#define WEBSERVER_H_INCLUDE


#include <ESP8266WebServer.h>


typedef void(*route_handler_t)(ESP8266WebServer&);


class APIRequestHandler : public RequestHandler
{
public:
  APIRequestHandler() : RequestHandler(), numRoutes(0) {}

  bool canHandle(HTTPMethod method, const String& uri) override;

  bool handle(ESP8266WebServer& server, HTTPMethod method, const String& uri) override;

  void addRoute(const char* const path, route_handler_t handler);

private:
  struct Route
  {
    Route() {}

    const char* path;

    route_handler_t handler;
  };

  static const size_t MAX_ROUTES = 10;
  size_t numRoutes;
  Route routes[MAX_ROUTES];
};


class FileRequestHandler : public RequestHandler
{
public:
  bool canHandle(HTTPMethod method, const String& uri) override;

  bool handle(ESP8266WebServer& server, HTTPMethod method, const String& uri) override;
};


class UploadRequestHandler : public RequestHandler
{
public:
  bool canHandle(HTTPMethod method, const String& uri) override;
  bool canUpload(const String& uri) override;

  void upload(ESP8266WebServer& server, const String& uri, HTTPUpload& upload) override;
};


#endif
