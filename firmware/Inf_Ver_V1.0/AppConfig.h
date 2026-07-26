#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

class AppConfig {
 private:
  Preferences prefs;

  String sanitizeDeviceName(const String& rawName) const;
  String buildUniqueDeviceName(const char* prefix) const;

 public:
  // ===== FIWARE / ORION =====
  // La entidad debe existir previamente. El firmware solamente hace PATCH.
  String serverUrl = "http://10.38.35.216:1026/v2/entities/tars-riego-01/attrs";

  // Nombre único usado por mDNS y por el AP de respaldo.
  String hostname = "";
  bool customDeviceName = false;

  unsigned long intervaloEnvio = 15000;
  unsigned long intervaloReintento = 20000;
  unsigned long intervaloSensores = 2000;

  // ===== KEYROCK =====
  String tokenUrl = "http://10.38.35.216:3001/oauth2/token";
  String clientId = "d4eac061-b057-45ff-87b2-f317275c3f58";
  String clientSecret = "f8e1bbf2-cbe9-44fb-a500-5bc6d60d17c7";
  String keyrockUser = "Tarst_v1@gmail.com";
  String keyrockPass = "123";
  bool skipToken = false;

  // ===== CALIBRACIÓN DEL CAUDALÍMETRO =====
  float pulsosPorLitro = 288.0f;

  void begin();
  void save();
  void reset();

  bool setDeviceName(const String& rawName);
  String getSetupSSID() const;
  String getLocalUrl() const;
};

extern AppConfig appConfig;

#endif
