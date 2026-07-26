#include "ValveManager.h"

#include "SensorManager.h"

// ============================================================
// DESENERGIZAR PUENTE H
// ============================================================

void ValveManager::stopBridge() {
  digitalWrite(VALVE_IN3_PIN, LOW);
  digitalWrite(VALVE_IN4_PIN, LOW);
}

// ============================================================
// PULSO POSITIVO: ABRIR
// ============================================================

void ValveManager::pulseOpen() {
  // Polaridad de apertura:
  // IN3 = HIGH
  // IN4 = LOW

  digitalWrite(VALVE_IN3_PIN, HIGH);
  digitalWrite(VALVE_IN4_PIN, LOW);

  delay(VALVE_PULSE_MS);

  stopBridge();
}

// ============================================================
// PULSO NEGATIVO: CERRAR
// ============================================================

void ValveManager::pulseClose() {
  // Polaridad inversa:
  // IN3 = LOW
  // IN4 = HIGH

  digitalWrite(VALVE_IN3_PIN, LOW);
  digitalWrite(VALVE_IN4_PIN, HIGH);

  delay(VALVE_PULSE_MS);

  stopBridge();
}

// ============================================================
// INICIALIZACIÓN
// ============================================================

void ValveManager::begin() {
  pinMode(VALVE_IN3_PIN, OUTPUT);
  pinMode(VALVE_IN4_PIN, OUTPUT);

  stopBridge();

  // El ESP32 no puede conocer físicamente la posición después
  // de perder energía. Por seguridad asumimos cerrada.
  openState = false;

  // El caudalímetro empieza deshabilitado.
  sensorManager.setFlowEnabled(false);

  Serial.println("[Valvula] Puente H inicializado");
  Serial.printf(
    "[Valvula] IN3 GPIO %d | IN4 GPIO %d\n",
    VALVE_IN3_PIN,
    VALVE_IN4_PIN
  );
  Serial.printf(
    "[Valvula] Pulso configurado: %d ms\n",
    VALVE_PULSE_MS
  );
}

// ============================================================
// ABRIR
// ============================================================

bool ValveManager::open() {
  if (openState) {
    Serial.println("[Valvula] Ya estaba marcada como abierta");
    return true;
  }

  Serial.println("[Valvula] Aplicando pulso de apertura");

  pulseOpen();

  openState = true;

  // Se activa el caudalímetro solamente después
  // del pulso de apertura.
  sensorManager.setFlowEnabled(true);

  Serial.println("[Valvula] ABIERTA");
  return true;
}

// ============================================================
// CERRAR
// ============================================================

bool ValveManager::close() {
  // Se deshabilita primero el caudalímetro para que no registre
  // pulsos durante el cierre.
  sensorManager.setFlowEnabled(false);

  if (!openState) {
    stopBridge();
    Serial.println("[Valvula] Ya estaba marcada como cerrada");
    return true;
  }

  Serial.println("[Valvula] Aplicando pulso de cierre");

  pulseClose();

  openState = false;

  Serial.println("[Valvula] CERRADA");
  return true;
}

// ============================================================
// CAMBIAR ESTADO
// ============================================================

bool ValveManager::setOpen(bool shouldOpen) {
  return shouldOpen ? open() : close();
}