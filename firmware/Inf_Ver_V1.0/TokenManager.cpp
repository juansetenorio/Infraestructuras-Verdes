#include "TokenManager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "AppConfig.h"
#include "WiFiManager.h"

String TokenManager::urlEncode(const String& value) {
  String encoded;
  char hex[4];

  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (isalnum(static_cast<unsigned char>(c)) ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      snprintf(hex, sizeof(hex), "%%%02X",
               static_cast<unsigned char>(c));
      encoded += hex;
    }
  }
  return encoded;
}

bool TokenManager::requestToken() {
  if (!wifiManager.isConnected()) {
    Serial.println("[Token] Sin WiFi");
    return false;
  }

  HTTPClient http;
  WiFiClientSecure secureClient;
  bool https = appConfig.tokenUrl.startsWith("https://");

  if (https) {
    secureClient.setInsecure();
    if (!http.begin(secureClient, appConfig.tokenUrl)) {
      Serial.println("[Token] No se pudo abrir URL HTTPS");
      return false;
    }
  } else {
    if (!http.begin(appConfig.tokenUrl)) {
      Serial.println("[Token] No se pudo abrir URL HTTP");
      return false;
    }
  }

  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "grant_type=password";
  body += "&username=" + urlEncode(appConfig.keyrockUser);
  body += "&password=" + urlEncode(appConfig.keyrockPass);
  body += "&client_id=" + urlEncode(appConfig.clientId);
  body += "&client_secret=" + urlEncode(appConfig.clientSecret);

  Serial.println("[Token] Solicitando token...");
  int httpCode = http.POST(body);

  if (httpCode >= 200 && httpCode < 300) {
    String response = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error && doc["access_token"].is<String>()) {
      token = doc["access_token"].as<String>();
      unsigned long expiresIn = doc["expires_in"] | 3600UL;
      unsigned long margin = expiresIn > 120 ? 60 : 5;
      tokenExpiry = millis() + (expiresIn - margin) * 1000UL;

      Serial.printf("[Token] Token recibido. Vigencia: %lu s\n", expiresIn);
      http.end();
      return true;
    }

    Serial.printf("[Token] JSON inválido: %s\n", error.c_str());
  } else {
    Serial.printf("[Token] Error HTTP %d\n", httpCode);
  }

  http.end();
  return false;
}

bool TokenManager::ensureValidToken() {
  if (hasToken()) return true;
  return requestToken();
}

bool TokenManager::hasToken() const {
  return token.length() > 0 &&
         static_cast<long>(tokenExpiry - millis()) > 0;
}

void TokenManager::clear() {
  token = "";
  tokenExpiry = 0;
  Serial.println("[Token] Token eliminado");
}
