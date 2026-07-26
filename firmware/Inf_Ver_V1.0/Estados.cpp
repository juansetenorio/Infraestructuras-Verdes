#include "Estados.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "AppConfig.h"
#include "PayloadBuilder.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include "TokenManager.h"
#include "ValveManager.h"
#include "WiFiManager.h"

// ============================================================
// INICIO
// ============================================================

void EstadoINICIO::onEnter() {
  executed = false;
  Serial.println("[Estado] INICIO");
}

void EstadoINICIO::execute() {
  if (executed) return;
  executed = true;

  unsigned long now = millis();
  statemachine->clocks.nextSend = now + appConfig.intervaloEnvio;

  Serial.printf("[INICIO] Próximo envío en %lu ms\n",
                appConfig.intervaloEnvio);

  statemachine->requestState(new EstadoLECTURA());
}

void EstadoINICIO::onExit() {
  Serial.println("[Estado] Saliendo de INICIO");
}

// ============================================================
// LECTURA
// ============================================================

void EstadoLECTURA::onEnter() {
  Serial.println("[Estado] LECTURA");
}

void EstadoLECTURA::execute() {
  sensorManager.update();

  unsigned long now = statemachine->clocks.now;
  if (static_cast<long>(now - statemachine->clocks.nextSend) >= 0) {
    statemachine->requestState(new EstadoENVIO());
  }
}

void EstadoLECTURA::onExit() {
  Serial.println("[Estado] Saliendo de LECTURA");
}

// ============================================================
// ENVÍO
// ============================================================

void EstadoENVIO::onEnter() {
  executed = false;
  Serial.println("[Estado] ENVIO");
}

int EstadoENVIO::sendPayloadOnce(const String& payload) {
  HTTPClient http;
  WiFiClientSecure secureClient;
  bool https = appConfig.serverUrl.startsWith("https://");

  bool begun = false;
  if (https) {
    secureClient.setInsecure();
    begun = http.begin(secureClient, appConfig.serverUrl);
  } else {
    begun = http.begin(appConfig.serverUrl);
  }

  if (!begun) {
    Serial.println("[ENVIO] No se pudo iniciar HTTPClient");
    return -1000;
  }

  http.setTimeout(12000);
  http.addHeader("Content-Type", "application/json");

  if (!appConfig.skipToken) {
    http.addHeader("Authorization", "Bearer " + tokenManager.getToken());
  }

  int code = http.PATCH(payload);

  if (code > 0) {
    String response = http.getString();
    if (response.length() > 0) {
      Serial.printf("[ENVIO] Respuesta: %s\n",
                    response.substring(0, 300).c_str());
    }
  }

  http.end();
  return code;
}

int EstadoENVIO::sendPayload(const String& payload) {
  int code = sendPayloadOnce(payload);

  // Un 401 invalida el token. Se renueva una vez y se reintenta el mismo payload.
  if (code == 401 && !appConfig.skipToken) {
    Serial.println("[ENVIO] Token rechazado; renovando");
    tokenManager.clear();

    if (tokenManager.ensureValidToken()) {
      code = sendPayloadOnce(payload);
    }
  }

  return code;
}

void EstadoENVIO::execute() {
  if (executed) return;
  executed = true;

  unsigned long now = millis();
  bool success = false;
  int httpCode = 0;
  String message;

  if (!wifiManager.isConnected()) {
    httpCode = -1;
    message = "Sin conexión WiFi; envío pospuesto";
    Serial.println("[ENVIO] Sin WiFi");
  } else if (!appConfig.skipToken && !tokenManager.ensureValidToken()) {
    httpCode = -2;
    message = "No se pudo obtener token Keyrock";
    Serial.println("[ENVIO] Token no disponible");
  } else {
    SensorData data = sensorManager.snapshot();
    String payload = construirPayload(data, valveManager.isOpen());

    Serial.println("[ENVIO] Payload NGSI v2:");
    Serial.println(payload);

    httpCode = sendPayload(payload);
    success = httpCode >= 200 && httpCode < 300;

    if (success) {
      message = "Datos enviados correctamente";
      Serial.printf("[ENVIO] Correcto: HTTP %d\n", httpCode);
    } else {
      message = "Fallo de envío HTTP " + String(httpCode);
      Serial.printf("[ENVIO] Falló: HTTP %d\n", httpCode);
    }
  }

  statemachine->sendStatus.lastSendOk = success;
  statemachine->sendStatus.lastHttpCode = httpCode;
  statemachine->sendStatus.lastSendAt = now;
  statemachine->sendStatus.lastMessage = message;

  statemachine->clocks.nextSend =
      now + (success ? appConfig.intervaloEnvio
                     : appConfig.intervaloReintento);

  statemachine->requestState(new EstadoLECTURA());
}

void EstadoENVIO::onExit() {
  Serial.println("[Estado] Saliendo de ENVIO");
}
