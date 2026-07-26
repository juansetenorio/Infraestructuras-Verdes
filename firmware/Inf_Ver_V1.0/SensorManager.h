#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// ===== PINES DEL HARDWARE =====
#define I2C_0_SDA 4
#define I2C_0_SCL 5
#define I2C_1_SDA 8
#define I2C_1_SCL 9
#define ONE_WIRE_BUS 10
#define FLOW_SENSOR_PIN 11

#define RS485_RO 18
#define RS485_DI 17
#define RS485_DE_RE 16

#define SOIL_ADDR 0x01
#define SOIL_BAUD 9600

#define SHT31_ADDR_0 0x44
#define SHT31_ADDR_1 0x44

struct SensorData {
  // Caudalímetro
  float flowRateLMin = 0.0f;
  float totalLiters = 0.0f;
  float flowFreqHz = 0.0f;
  uint32_t pulsesLastWindow = 0;
  bool flowEnabled = false;

  // DS18B20
  float ambientTempC = -99.0f;
  bool dsOk = false;

  // SHT31 bus 0
  float sht0TempC = -99.0f;
  float sht0HumPct = -99.0f;
  bool sht0Ok = false;

  // SHT31 bus 1
  float sht1TempC = -99.0f;
  float sht1HumPct = -99.0f;
  bool sht1Ok = false;

  // Sensor de suelo RS485
  float soilTempC = -99.0f;
  float soilHumPct = -99.0f;
  bool soilOk = false;
};

class SensorManager {
 private:
  SensorData data;
  Preferences flowPrefs;

  unsigned long lastFlowCalculation = 0;
  unsigned long lastSlowCycle = 0;
  unsigned long dsRequestAt = 0;
  unsigned long lastFlowPersist = 0;

  bool dsConversionPending = false;
  uint32_t lastPersistedPulseCount = 0;

  static constexpr unsigned long FLOW_WINDOW_MS = 1000;
  static constexpr unsigned long DS_CONVERSION_MS = 750;
  static constexpr unsigned long FLOW_PERSIST_MS = 60000;

  void updateFlow(unsigned long now);
  void startSlowSensorCycle(unsigned long now);
  void finishDS18B20(unsigned long now);

  bool readSHT31(uint8_t busIndex, uint8_t address,
                 float& temperature, float& humidity);
  bool readSoilSensor(float& temperature, float& humidity);
  uint8_t sht31CRC8(const uint8_t* bytes, int length);
  uint16_t modbusCRC16(const uint8_t* buffer, int length);

 public:
  void begin();
  void update();

  SensorData snapshot() const { return data; }

  void setFlowEnabled(bool enabled);
  bool isFlowEnabled() const { return data.flowEnabled; }

  void resetFlowTotal();
  void persistFlowTotal();

  void i2cScan();
};

extern SensorManager sensorManager;

#endif
