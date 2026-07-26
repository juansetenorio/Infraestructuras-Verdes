#include "AppConfig.h"

String AppConfig::sanitizeDeviceName(const String& rawName) const {
  String cleaned;
  cleaned.reserve(24);
  bool previousWasDash = false;

  for (size_t i = 0; i < rawName.length() && cleaned.length() < 24; i++) {
    char c = rawName.charAt(i);
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';

    const bool alphanumeric =
        (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    const bool separator = c == '-' || c == '_' || c == ' ';

    if (alphanumeric) {
      cleaned += c;
      previousWasDash = false;
    } else if (separator && cleaned.length() > 0 && !previousWasDash) {
      cleaned += '-';
      previousWasDash = true;
    }
  }

  while (cleaned.length() > 0 && cleaned.charAt(cleaned.length() - 1) == '-') {
    cleaned.remove(cleaned.length() - 1);
  }

  return cleaned;
}

String AppConfig::buildUniqueDeviceName(const char* prefix) const {
  const uint64_t chipId = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06llx",
           static_cast<unsigned long long>(chipId & 0xFFFFFFULL));

  String name = sanitizeDeviceName(prefix);
  name += '-';
  name += suffix;
  return name;
}

void AppConfig::begin() {
  prefs.begin("tarsrigcfg", false);

  serverUrl = prefs.getString("srvurl", serverUrl);

  const String storedHostname = prefs.getString("host", "");
  customDeviceName = prefs.getBool("customname", false);
  if (customDeviceName) {
    hostname = sanitizeDeviceName(storedHostname);
    if (hostname.length() < 3) customDeviceName = false;
  }
  if (!customDeviceName) {
    hostname = buildUniqueDeviceName("tars-riego");
  }

  intervaloEnvio = prefs.getULong("sendms", intervaloEnvio);
  intervaloReintento = prefs.getULong("retryms", intervaloReintento);
  intervaloSensores = prefs.getULong("readms", intervaloSensores);

  tokenUrl = prefs.getString("tokenurl", tokenUrl);
  clientId = prefs.getString("clientid", clientId);
  clientSecret = prefs.getString("clientsec", clientSecret);
  keyrockUser = prefs.getString("kruser", keyrockUser);
  keyrockPass = prefs.getString("krpass", keyrockPass);
  skipToken = prefs.getBool("skiptoken", skipToken);

  pulsosPorLitro = prefs.getFloat("ppl", pulsosPorLitro);

  prefs.putString("host", hostname);
  prefs.putBool("customname", customDeviceName);
  prefs.end();

  if (intervaloEnvio < 5000) intervaloEnvio = 5000;
  if (intervaloReintento < 5000) intervaloReintento = 5000;
  if (intervaloSensores < 1000) intervaloSensores = 1000;
  if (pulsosPorLitro < 1.0f) pulsosPorLitro = 288.0f;

  Serial.println("[AppConfig] Configuración cargada");
  Serial.printf("[AppConfig] Dispositivo: %s\n", hostname.c_str());
  Serial.printf("[AppConfig] URL local: %s\n", getLocalUrl().c_str());
  Serial.printf("[AppConfig] AP respaldo: %s\n", getSetupSSID().c_str());
  Serial.printf("[AppConfig] URL FIWARE: %s\n", serverUrl.c_str());
  Serial.printf("[AppConfig] Envío: %lu ms | Sensores: %lu ms\n",
                intervaloEnvio, intervaloSensores);
  Serial.printf("[AppConfig] Calibración: %.3f pulsos/L\n", pulsosPorLitro);
}

void AppConfig::save() {
  prefs.begin("tarsrigcfg", false);

  prefs.putString("srvurl", serverUrl);
  prefs.putString("host", hostname);
  prefs.putBool("customname", customDeviceName);
  prefs.putULong("sendms", intervaloEnvio);
  prefs.putULong("retryms", intervaloReintento);
  prefs.putULong("readms", intervaloSensores);

  prefs.putString("tokenurl", tokenUrl);
  prefs.putString("clientid", clientId);
  prefs.putString("clientsec", clientSecret);
  prefs.putString("kruser", keyrockUser);
  prefs.putString("krpass", keyrockPass);
  prefs.putBool("skiptoken", skipToken);

  prefs.putFloat("ppl", pulsosPorLitro);
  prefs.end();

  Serial.println("[AppConfig] Configuración guardada en NVS");
}

void AppConfig::reset() {
  prefs.begin("tarsrigcfg", false);
  prefs.clear();
  prefs.end();

  serverUrl = "http://10.38.35.216:1026/v2/entities/tars-riego-01/attrs";
  customDeviceName = false;
  hostname = buildUniqueDeviceName("tars-riego");
  intervaloEnvio = 15000;
  intervaloReintento = 20000;
  intervaloSensores = 2000;

  tokenUrl = "http://10.38.35.216:3001/oauth2/token";
  clientId = "d4eac061-b057-45ff-87b2-f317275c3f58";
  clientSecret = "f8e1bbf2-cbe9-44fb-a500-5bc6d60d17c7";
  keyrockUser = "Tarst_v1@gmail.com";
  keyrockPass = "123";
  skipToken = false;

  pulsosPorLitro = 288.0f;
  Serial.println("[AppConfig] Defaults restaurados");
}

bool AppConfig::setDeviceName(const String& rawName) {
  const String cleaned = sanitizeDeviceName(rawName);
  if (cleaned.length() < 3) return false;

  hostname = cleaned;
  customDeviceName = true;
  return true;
}

String AppConfig::getSetupSSID() const {
  String ssid = hostname;
  ssid.toUpperCase();
  ssid += "-SETUP";
  if (ssid.length() > 31) ssid = ssid.substring(0, 31);
  return ssid;
}

String AppConfig::getLocalUrl() const {
  return "http://" + hostname + ".local";
}
