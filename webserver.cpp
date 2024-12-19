#include "webserver.h"


#include <LittleFS.h>


String listing(String path)
{
  String html = "<html><head><title>Directory Listing</title></head><body>";
  html += "<h1>Directory Listing for " + path + "</h1><ul>";
  Dir dir = LittleFS.openDir(path);
  while (dir.next())
  {
    String name = dir.fileName();
    String display = dir.isDirectory() ? name + "/" : name;
    String href = path + (path.endsWith("/") ? "" : "/") + name;
    html += "<li><a href=\"" + href + "\">" + display + "</a></ul>";
  }
  html += "</ul></body></html>";
  return html;
}


void serve(ESP8266WebServer& server, String path)
{
  File file = LittleFS.open(path == "/" ? "/index.html" : path, "r");
  if (!file)
  {
    if (path == "/upload.html")
    {
      server.send(200, "text/html", R"(<!DOCTYPE html>
<html>
  <head>
    <title>Dodge Cube</title>

    <link rel="stylesheet" type="text/css" href="/style.css">
  </head>
  <body>
    <form method="POST" action="/do-upload.html" enctype="multipart/form-data">
      <input type="file" name="data"/>
      <input type="submit" value="Upload"/>
    </form>
  </body>
</html>)");
      return;
    }
    server.send(404, "text/html", listing(path));
    return;
  }

  server.setContentLength(file.size());
  server.send(200, mime::getContentType(file.name()), "");
  uint8_t buffer[512];
  size_t read;
  while ((read = file.read(buffer, sizeof(buffer))) > 0)
    server.client().write(buffer, read);
  file.close();
  // char buf[file.size() + 1];
  // file.readBytes(buf, file.size());
  // buf[file.size()] = 0;
  // file.close();

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

  serve(server, uri);
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
