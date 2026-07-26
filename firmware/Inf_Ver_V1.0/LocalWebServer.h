#ifndef LOCAL_WEB_SERVER_H
#define LOCAL_WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>

class LocalWebServer {
 private:
  WebServer* server;
  bool initialized = false;
  bool mdnsStarted = false;

  bool restartScheduled = false;
  unsigned long restartAt = 0;

  bool otaError = false;
  bool otaStarted = false;

  void registerRoutes();
  void handleRoot();
  void handleData();
  void handleConfigPage();
  void handleConfigSave();
  void handleValve();
  void handleFlowReset();

  String htmlEscape(const String& input);
  void sendJsonMessage(int code, bool ok, const String& message);

 public:
  explicit LocalWebServer(WebServer* srv) : server(srv) {}

  void begin();
  void handle();
};

extern LocalWebServer localWebServer;

#endif
