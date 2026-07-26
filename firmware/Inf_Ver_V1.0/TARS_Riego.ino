#include <Arduino.h>
#include <WebServer.h>

#include "AppConfig.h"
#include "Estados.h"
#include "LocalWebServer.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include "TokenManager.h"
#include "ValveManager.h"
#include "WiFiManager.h"

// Servidor HTTP local
WebServer server(80);

// Objetos globales del firmware
AppConfig appConfig;
WiFiManager wifiManager;
TokenManager tokenManager;
SensorManager sensorManager;
ValveManager valveManager;
StateMachine stateMachine;
LocalWebServer localWebServer(&server);

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("========================================");
  Serial.println(" TARS RIEGO - INICIO DEL FIRMWARE");
  Serial.println("========================================");

  appConfig.begin();
  // La electroválvula se inicializa cerrada por seguridad.
  sensorManager.begin();
  valveManager.begin();

  // La web se publica incluso si no hay conexión al router:
  // en ese caso se levanta el AP de configuración.
  wifiManager.begin();
  localWebServer.begin();

  stateMachine.begin(new EstadoINICIO());
}

void loop() {
  wifiManager.handle();
  localWebServer.handle();
  stateMachine.update();

  // Cede tiempo al RTOS/WiFi sin bloquear el conteo por interrupción.
  delay(1);
}
