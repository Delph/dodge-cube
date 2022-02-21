#include "webserver.h"

#include <LittleFS.h>

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


bool APIRequestHandler::canHandle(HTTPMethod method, const String& uri)
{
  return method == HTTP_POST && uri.startsWith("/api");
}

bool APIRequestHandler::handle(ESP8266WebServer& server, HTTPMethod method, const String& uri)
{
  const String path = uri.substring(5);

  for (size_t i = 0; i < numRoutes; ++i)
  {
    if (path == routes[i].path)
    {
      routes[i].handler(server);
      return true;
    }
  }

  return false;
}

void APIRequestHandler::addRoute(const char* const path, route_handler_t handler)
{
  if (numRoutes >= MAX_ROUTES)
    return;

  routes[numRoutes].path = path;
  routes[numRoutes].handler = handler;
  ++numRoutes;
}


bool FileRequestHandler::canHandle(HTTPMethod method, const String& uri)
{
  return method == HTTP_GET && uri.startsWith("/api") == false;
}


bool FileRequestHandler::handle(ESP8266WebServer& server, HTTPMethod method, const String& uri)
{
  if (!canHandle(method, uri))
    return false;

  const char* path = uri == "/" ? "/index.html" : uri.c_str();
  serve(server, path);
  return true;
}



bool UploadRequestHandler::canHandle(HTTPMethod method, const String& uri)
{
  return method == HTTP_POST && uri == "/do-upload.html";
}


bool UploadRequestHandler::canUpload(const String& uri)
{
  return uri == "/do-upload.html";
}


void UploadRequestHandler::upload(ESP8266WebServer& server, const String& uri, HTTPUpload& upload)
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
