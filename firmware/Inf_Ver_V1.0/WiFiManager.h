#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

class WiFiManager {
 private:
  Preferences prefs;
  String currentSSID;
  String currentPass;
  String activeAPSSID;

  bool apActive = false;
  unsigned long lastReconnectAttempt = 0;

  static constexpr unsigned long RECONNECT_INTERVAL = 30000;

  const char* defaultSSID = "Claro_4A68C6";
  const char* defaultPass = "48668438";
  const char* apPass = "12345678";

 public:
  void begin();
  void handle();

  bool connect(int maxAttempts = 20);
  void createAP();
  void saveCredentials(const String& newSSID, const String& newPass);
  void reset();

  bool isConnected() const;
  bool isAPActive() const { return apActive; }
  String getIP() const;
  String getSSID() const { return currentSSID; }
  String getAPSSID() const { return activeAPSSID; }
};

extern WiFiManager wifiManager;

#endif
