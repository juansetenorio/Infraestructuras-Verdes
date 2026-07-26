#include "SensorManager.h"

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>

#include "AppConfig.h"

// Segundo bus I2C y UART RS485
static TwoWire I2C_Bus1(1);
static HardwareSerial RS485Serial(1);

// DS18B20
static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature dsSensors(&oneWire);

// Variables compartidas con la ISR
static portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t isrWindowPulses = 0;
static volatile uint32_t isrTotalPulses = 0;
static volatile bool isrFlowEnabled = false;

static void IRAM_ATTR countFlowPulse() {
  if (!isrFlowEnabled) return;

  portENTER_CRITICAL_ISR(&flowMux);
  isrWindowPulses++;
  isrTotalPulses++;
  portEXIT_CRITICAL_ISR(&flowMux);
}

void SensorManager::begin() {
  // I2C bus 0
  Wire.begin(I2C_0_SDA, I2C_0_SCL);
  Wire.setClock(100000);

  // I2C bus 1
  I2C_Bus1.begin(I2C_1_SDA, I2C_1_SCL);
  I2C_Bus1.setClock(100000);

  // RS485
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485Serial.begin(SOIL_BAUD, SERIAL_8N1, RS485_RO, RS485_DI);

  // Caudalímetro: se configura, pero NO se conecta la interrupción todavía.
  // La interrupción se habilita únicamente al abrir la electroválvula.
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  // DS18B20
  dsSensors.begin();
  dsSensors.setResolution(12);
  dsSensors.setWaitForConversion(false);
  dsSensors.requestTemperatures();
  dsRequestAt = millis();
  dsConversionPending = true;

  // Restaurar total de pulsos
  flowPrefs.begin("tarsrigflow", false);
  uint32_t storedPulses = flowPrefs.getUInt("total", 0);
  portENTER_CRITICAL(&flowMux);
  isrTotalPulses = storedPulses;
  isrWindowPulses = 0;
  isrFlowEnabled = false;
  portEXIT_CRITICAL(&flowMux);

  lastPersistedPulseCount = storedPulses;
  data.totalLiters = storedPulses / appConfig.pulsosPorLitro;

  i2cScan();

  // Primera lectura del suelo para diagnóstico
  data.soilOk = readSoilSensor(data.soilTempC, data.soilHumPct);

  lastFlowCalculation = millis();
  lastSlowCycle = millis();

  Serial.printf("[Sensores] Caudalímetro GPIO %d, inicialmente DESHABILITADO\n",
                FLOW_SENSOR_PIN);
  Serial.printf("[Sensores] Total restaurado: %.3f L\n", data.totalLiters);
}

void SensorManager::update() {
  unsigned long now = millis();

  updateFlow(now);
  finishDS18B20(now);

  if (now - lastSlowCycle >= appConfig.intervaloSensores) {
    startSlowSensorCycle(now);
    lastSlowCycle = now;
  }

  if (now - lastFlowPersist >= FLOW_PERSIST_MS) {
    persistFlowTotal();
    lastFlowPersist = now;
  }
}

void SensorManager::setFlowEnabled(bool enabled) {
  if (enabled == data.flowEnabled) return;

  if (enabled) {
    portENTER_CRITICAL(&flowMux);
    isrWindowPulses = 0;
    isrFlowEnabled = true;
    portEXIT_CRITICAL(&flowMux);

    lastFlowCalculation = millis();
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN),
                    countFlowPulse, FALLING);

    data.flowEnabled = true;
    data.flowRateLMin = 0.0f;
    data.flowFreqHz = 0.0f;
    data.pulsesLastWindow = 0;

    Serial.println("[Caudal] Habilitado: válvula abierta");
  } else {
    // Primero se desconecta la interrupción y después se limpia la ventana.
    detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));

    portENTER_CRITICAL(&flowMux);
    isrFlowEnabled = false;
    isrWindowPulses = 0;
    portEXIT_CRITICAL(&flowMux);

    data.flowEnabled = false;
    data.flowRateLMin = 0.0f;
    data.flowFreqHz = 0.0f;
    data.pulsesLastWindow = 0;

    persistFlowTotal();
    Serial.println("[Caudal] Deshabilitado: válvula cerrada");
  }
}

void SensorManager::updateFlow(unsigned long now) {
  if (!data.flowEnabled) {
    data.flowRateLMin = 0.0f;
    data.flowFreqHz = 0.0f;
    data.pulsesLastWindow = 0;

    uint32_t totalCopy;
    portENTER_CRITICAL(&flowMux);
    totalCopy = isrTotalPulses;
    portEXIT_CRITICAL(&flowMux);

    data.totalLiters = totalCopy / appConfig.pulsosPorLitro;
    return;
  }

  unsigned long elapsed = now - lastFlowCalculation;
  if (elapsed < FLOW_WINDOW_MS) return;

  uint32_t windowPulses;
  uint32_t totalPulses;

  portENTER_CRITICAL(&flowMux);
  windowPulses = isrWindowPulses;
  isrWindowPulses = 0;
  totalPulses = isrTotalPulses;
  portEXIT_CRITICAL(&flowMux);

  data.pulsesLastWindow = windowPulses;
  data.flowFreqHz = windowPulses * 1000.0f / elapsed;
  data.flowRateLMin =
      (data.flowFreqHz * 60.0f) / appConfig.pulsosPorLitro;
  data.totalLiters = totalPulses / appConfig.pulsosPorLitro;

  lastFlowCalculation = now;

  Serial.printf("[Caudal] %.3f L/min | %.2f Hz | total %.3f L\n",
                data.flowRateLMin, data.flowFreqHz, data.totalLiters);
}

void SensorManager::startSlowSensorCycle(unsigned long now) {
  // Se inicia la conversión DS18B20 y se leerá más tarde, sin bloquear 750 ms.
  dsSensors.requestTemperatures();
  dsRequestAt = now;
  dsConversionPending = true;

  data.sht0Ok = readSHT31(0, SHT31_ADDR_0,
                          data.sht0TempC, data.sht0HumPct);
  data.sht1Ok = readSHT31(1, SHT31_ADDR_1,
                          data.sht1TempC, data.sht1HumPct);
  data.soilOk = readSoilSensor(data.soilTempC, data.soilHumPct);
}

void SensorManager::finishDS18B20(unsigned long now) {
  if (!dsConversionPending) return;
  if (now - dsRequestAt < DS_CONVERSION_MS) return;

  float value = dsSensors.getTempCByIndex(0);
  bool valid = value != DEVICE_DISCONNECTED_C &&
               value >= -55.0f && value <= 125.0f;

  if (valid) {
    data.ambientTempC = value;
    data.dsOk = true;
    Serial.printf("[DS18B20] %.2f °C\n", value);
  } else {
    data.dsOk = false;
    Serial.printf("[DS18B20] Lectura inválida: %.2f\n", value);
  }

  dsConversionPending = false;
}

uint8_t SensorManager::sht31CRC8(const uint8_t* bytes, int length) {
  uint8_t crc = 0xFF;

  for (int i = 0; i < length; i++) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
  }
  return crc;
}

bool SensorManager::readSHT31(uint8_t busIndex, uint8_t address,
                              float& temperature, float& humidity) {
  TwoWire& bus = busIndex == 0 ? Wire : I2C_Bus1;

  bus.beginTransmission(address);
  bus.write(0x24);
  bus.write(0x00);
  if (bus.endTransmission() != 0) {
    Serial.printf("[SHT31 bus %u] Error TX\n", busIndex);
    return false;
  }

  delay(20);

  uint8_t bytes[6];
  int received = bus.requestFrom(address, static_cast<uint8_t>(6));
  if (received != 6) {
    while (bus.available()) bus.read();
    Serial.printf("[SHT31 bus %u] Error RX: %d bytes\n",
                  busIndex, received);
    return false;
  }

  for (int i = 0; i < 6; i++) bytes[i] = bus.read();

  if (sht31CRC8(bytes, 2) != bytes[2] ||
      sht31CRC8(bytes + 3, 2) != bytes[5]) {
    Serial.printf("[SHT31 bus %u] CRC inválido\n", busIndex);
    return false;
  }

  uint16_t rawTemp = (static_cast<uint16_t>(bytes[0]) << 8) | bytes[1];
  uint16_t rawHum = (static_cast<uint16_t>(bytes[3]) << 8) | bytes[4];

  temperature = -45.0f + 175.0f * rawTemp / 65535.0f;
  humidity = 100.0f * rawHum / 65535.0f;

  bool valid = temperature >= -40.0f && temperature <= 125.0f &&
               humidity >= 0.0f && humidity <= 100.0f;

  Serial.printf("[SHT31 bus %u] T=%.2f °C H=%.2f %%\n",
                busIndex, temperature, humidity);
  return valid;
}

uint16_t SensorManager::modbusCRC16(const uint8_t* buffer, int length) {
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < length; pos++) {
    crc ^= static_cast<uint16_t>(buffer[pos]);

    for (int bit = 0; bit < 8; bit++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool SensorManager::readSoilSensor(float& temperature, float& humidity) {
  uint8_t request[8] = {
    SOIL_ADDR, 0x03, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00
  };

  uint16_t crc = modbusCRC16(request, 6);
  request[6] = crc & 0xFF;
  request[7] = (crc >> 8) & 0xFF;

  while (RS485Serial.available()) RS485Serial.read();

  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(50);
  RS485Serial.write(request, sizeof(request));
  RS485Serial.flush();
  delayMicroseconds(100);
  digitalWrite(RS485_DE_RE, LOW);

  uint8_t response[9];
  int index = 0;
  unsigned long startedAt = millis();

  while (index < 9 && millis() - startedAt < 300) {
    if (RS485Serial.available()) {
      response[index++] = RS485Serial.read();
    }
    delay(1);
  }

  if (index != 9) {
    Serial.printf("[Suelo] Timeout: %d/9 bytes\n", index);
    return false;
  }

  if (response[0] != SOIL_ADDR ||
      response[1] != 0x03 ||
      response[2] != 0x04) {
    Serial.println("[Suelo] Trama inválida");
    return false;
  }

  uint16_t calculatedCRC = modbusCRC16(response, 7);
  uint16_t receivedCRC = response[7] |
                         (static_cast<uint16_t>(response[8]) << 8);

  if (calculatedCRC != receivedCRC) {
    Serial.println("[Suelo] Error CRC");
    return false;
  }

  uint16_t rawHumidity =
      (static_cast<uint16_t>(response[3]) << 8) | response[4];
  int16_t rawTemperature =
      static_cast<int16_t>(
          (static_cast<uint16_t>(response[5]) << 8) | response[6]);

  humidity = rawHumidity / 10.0f;
  temperature = rawTemperature / 10.0f;

  bool valid = humidity >= 0.0f && humidity <= 100.0f &&
               temperature >= -40.0f && temperature <= 85.0f;

  Serial.printf("[Suelo] T=%.1f °C H=%.1f %%\n",
                temperature, humidity);
  return valid;
}

void SensorManager::resetFlowTotal() {
  portENTER_CRITICAL(&flowMux);
  isrTotalPulses = 0;
  isrWindowPulses = 0;
  portEXIT_CRITICAL(&flowMux);

  data.totalLiters = 0.0f;
  data.flowRateLMin = 0.0f;
  data.flowFreqHz = 0.0f;
  data.pulsesLastWindow = 0;

  flowPrefs.putUInt("total", 0);
  lastPersistedPulseCount = 0;
  Serial.println("[Caudal] Total reiniciado");
}

void SensorManager::persistFlowTotal() {
  uint32_t totalCopy;

  portENTER_CRITICAL(&flowMux);
  totalCopy = isrTotalPulses;
  portEXIT_CRITICAL(&flowMux);

  if (totalCopy == lastPersistedPulseCount) return;

  flowPrefs.putUInt("total", totalCopy);
  lastPersistedPulseCount = totalCopy;
  Serial.printf("[Caudal] Total persistido: %u pulsos\n", totalCopy);
}

void SensorManager::i2cScan() {
  TwoWire* buses[] = { &Wire, &I2C_Bus1 };
  const char* names[] = { "Bus 0 SDA4/SCL5", "Bus 1 SDA8/SCL9" };

  for (int b = 0; b < 2; b++) {
    Serial.printf("[I2C] Escaneando %s\n", names[b]);
    int count = 0;

    for (uint8_t address = 1; address < 127; address++) {
      buses[b]->beginTransmission(address);
      if (buses[b]->endTransmission() == 0) {
        Serial.printf("  - dispositivo 0x%02X\n", address);
        count++;
      }
    }

    if (count == 0) Serial.println("  - sin dispositivos");
  }
}
