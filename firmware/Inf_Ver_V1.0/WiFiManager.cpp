#include "WiFiManager.h"

#include "AppConfig.h"

void WiFiManager::begin() {
  prefs.begin("tarsrigwifi", true);
  currentSSID = prefs.getString("ssid", defaultSSID);
  currentPass = prefs.getString("pass", defaultPass);
  prefs.end();

  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  if (!connect(20)) {
    createAP();
  }
}

bool WiFiManager::connect(int maxAttempts) {
  if (currentSSID.length() == 0) {
    Serial.println("[WiFi] No hay SSID configurado");
    return false;
  }

  WiFi.disconnect(false, false);
  delay(100);

  // Conexión a una red abierta o protegida.
  if (currentPass.length() == 0) {
    Serial.printf(
      "[WiFi] Conectando a red abierta: %s",
      currentSSID.c_str()
    );

    WiFi.begin(currentSSID.c_str());
  } else {
    Serial.printf(
      "[WiFi] Conectando a red protegida: %s",
      currentSSID.c_str()
    );

    WiFi.begin(
      currentSSID.c_str(),
      currentPass.c_str()
    );
  }

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < maxAttempts
  ) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(
      "[WiFi] Conectado. IP: %s\n",
      WiFi.localIP().toString().c_str()
    );

    // Apagar el AP cuando se conecte al router.
    if (apActive) {
      WiFi.softAPdisconnect(true);
      apActive = false;
      activeAPSSID = "";

      WiFi.mode(WIFI_STA);

      Serial.println("[WiFi] AP de configuración apagado");
    }

    return true;
  }

  Serial.println("[WiFi] No fue posible conectar al router");
  return false;
}

void WiFiManager::createAP() {
  if (apActive) return;

  WiFi.mode(WIFI_AP_STA);

  activeAPSSID = appConfig.getSetupSSID();

  apActive = WiFi.softAP(
    activeAPSSID.c_str(),
    apPass
  );

  if (apActive) {
    Serial.printf(
      "[WiFi] AP activo: %s\n",
      activeAPSSID.c_str()
    );

    Serial.printf(
      "[WiFi] Clave AP: %s\n",
      apPass
    );

    Serial.printf(
      "[WiFi] IP AP: %s\n",
      WiFi.softAPIP().toString().c_str()
    );
  } else {
    Serial.println("[WiFi] Error creando el AP de respaldo");
  }
}

void WiFiManager::handle() {
  // Cuando se conecte al router, apagar el AP.
  if (isConnected()) {
    if (apActive) {
      WiFi.softAPdisconnect(true);
      apActive = false;
      activeAPSSID = "";

      WiFi.mode(WIFI_STA);

      Serial.println("[WiFi] AP de configuración apagado");
    }

    return;
  }

  unsigned long now = millis();

  if (now - lastReconnectAttempt < RECONNECT_INTERVAL) {
    return;
  }

  lastReconnectAttempt = now;

  if (!apActive) {
    createAP();
  }

  if (currentSSID.length() == 0) {
    Serial.println("[WiFi] No hay SSID para reconectar");
    return;
  }

  Serial.println("[WiFi] Reintentando conexión STA...");

  WiFi.disconnect(false, false);
  delay(100);

  // Reconectar a una red abierta o protegida.
  if (currentPass.length() == 0) {
    Serial.printf(
      "[WiFi] Reintentando red abierta: %s\n",
      currentSSID.c_str()
    );

    WiFi.begin(currentSSID.c_str());
  } else {
    Serial.printf(
      "[WiFi] Reintentando red protegida: %s\n",
      currentSSID.c_str()
    );

    WiFi.begin(
      currentSSID.c_str(),
      currentPass.c_str()
    );
  }
}

void WiFiManager::saveCredentials(
  const String& newSSID,
  const String& newPass
) {
  currentSSID = newSSID;
  currentPass = newPass;

  currentSSID.trim();

  prefs.begin("tarsrigwifi", false);
  prefs.putString("ssid", currentSSID);
  prefs.putString("pass", currentPass);
  prefs.end();

  Serial.printf(
    "[WiFi] Credenciales guardadas para %s\n",
    currentSSID.c_str()
  );

  if (currentPass.length() == 0) {
    Serial.println("[WiFi] Red configurada como abierta");
  } else {
    Serial.println("[WiFi] Red configurada con contraseña");
  }
}

void WiFiManager::reset() {
  prefs.begin("tarsrigwifi", false);
  prefs.clear();
  prefs.end();

  currentSSID = defaultSSID;
  currentPass = defaultPass;

  Serial.println("[WiFi] Credenciales restauradas");
}

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() const {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }

  if (apActive) {
    return WiFi.softAPIP().toString();
  }

  return "Sin red";
}