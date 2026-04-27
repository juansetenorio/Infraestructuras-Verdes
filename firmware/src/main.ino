#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <ModbusMaster.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_SHT31.h>

// ===================== WIFI / SERVER =====================
const char* WIFI_SSID = "Claro_2C06BE";
const char* WIFI_PASS = "16652524";

const char* SERVER_URL = "http://192.168.0.110:5000/data";

// ===================== PINOUT ESP32-S3 =====================
#define I2C_SDA 8
#define I2C_SCL 9

#define MAX485_DE 16
#define TX2_PIN   17
#define RX2_PIN   18

#define ONE_WIRE_BUS 4
#define FLOW_PIN     5
#define VALVE_PIN    6

// SHT31 (2 sensores I2C)
const uint8_t SHT31_ADDR_LOW  = 0x44;
const uint8_t SHT31_ADDR_HIGH = 0x45;

// Flujo
const float PULSOS_POR_LITRO = 288.0f;

// Intervalos
const unsigned long readInterval = 2000;
const unsigned long sendInterval = 5000;

// ===================== OBJETOS =====================
HardwareSerial RS485Serial(1);

ModbusMaster node;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
Adafruit_SHT31 sht31_low  = Adafruit_SHT31();
Adafruit_SHT31 sht31_high = Adafruit_SHT31();

// ===================== ESTADO =====================
float soil_temp_1       = 0.0f;
float soil_humidity_1   = 0.0f;
float soil_temp_2       = 0.0f;
float air_temp_low      = 0.0f;
float air_humidity_low  = 0.0f;
float air_temp_high     = 0.0f;
float air_humidity_high = 0.0f;

float flow_litros_sesion = 0.0f;
float flow_litros_total  = 0.0f;
bool  valve_state        = false;

bool modbus_ok      = false;
bool ds18b20_ok     = false;
bool sht31_low_ok   = false;
bool sht31_high_ok  = false;

unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;

// ===================== FLUJO (INTERRUPT) =====================
volatile uint32_t pulsosFlujo = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR contarPulsos() {
  portENTER_CRITICAL_ISR(&mux);
  pulsosFlujo++;
  portEXIT_CRITICAL_ISR(&mux);
}

// ===================== RS485 =====================
void preTransmission()  { digitalWrite(MAX485_DE, HIGH); delay(2); }
void postTransmission() { delay(2); digitalWrite(MAX485_DE, LOW);  }

// ===================== VÁLVULA =====================
void setValvula(bool abrir) {
  valve_state = abrir;
  digitalWrite(VALVE_PIN, abrir ? HIGH : LOW);

  if (abrir) {
    portENTER_CRITICAL(&mux);
    pulsosFlujo = 0;
    portEXIT_CRITICAL(&mux);
    flow_litros_sesion = 0.0f;
    Serial.println("[VALVULA] ABIERTA");
  } else {
    Serial.printf("[VALVULA] CERRADA - Sesion: %.4f L\n", flow_litros_sesion);
  }
}

// ===================== SENSORES =====================
void leerModbus() {
  uint8_t res = node.readHoldingRegisters(2, 2);
  if (res == node.ku8MBSuccess) {
    soil_humidity_1 = node.getResponseBuffer(0) / 10.0f;
    soil_temp_1     = node.getResponseBuffer(1) / 10.0f;
    modbus_ok = true;
  } else {
    modbus_ok = false;
    Serial.printf("[MODBUS] Error: 0x%02X\n", res);
  }
}

void leerDS18B20() {
  ds18b20.requestTemperatures();
  float t = ds18b20.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) {
    soil_temp_2 = 0.0f;
    ds18b20_ok = false;
  } else {
    soil_temp_2 = t;
    ds18b20_ok = true;
  }
}

void leerSHT31() {
  float t1 = sht31_low.readTemperature();
  float h1 = sht31_low.readHumidity();
  if (!isnan(t1) && !isnan(h1)) {
    air_temp_low = t1;
    air_humidity_low = h1;
    sht31_low_ok = true;
  } else {
    air_temp_low = 0.0f;
    air_humidity_low = 0.0f;
    sht31_low_ok = false;
  }

  float t2 = sht31_high.readTemperature();
  float h2 = sht31_high.readHumidity();
  if (!isnan(t2) && !isnan(h2)) {
    air_temp_high = t2;
    air_humidity_high = h2;
    sht31_high_ok = true;
  } else {
    air_temp_high = 0.0f;
    air_humidity_high = 0.0f;
    sht31_high_ok = false;
  }
}

void calcularFlujo() {
  if (!valve_state) return;

  uint32_t pulsos = 0;
  portENTER_CRITICAL(&mux);
  pulsos = pulsosFlujo;
  pulsosFlujo = 0;
  portEXIT_CRITICAL(&mux);

  float litros = pulsos / PULSOS_POR_LITRO;
  flow_litros_sesion += litros;
  flow_litros_total  += litros;
}

// ===================== WIFI =====================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.printf("[WIFI] Conectando a %s", WIFI_SSID);
  uint8_t intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] OK IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] No conectado (seguira intentando)");
  }
}

// ===================== POST JSON =====================
bool postJSON(const String& json) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST((uint8_t*)json.c_str(), json.length());
  String resp = http.getString();
  http.end();

  Serial.printf("[POST] HTTP %d | resp: %s\n", code, resp.c_str());
  return (code >= 200 && code < 300);
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(400);

  // GPIO básicos
  pinMode(VALVE_PIN, OUTPUT);
  digitalWrite(VALVE_PIN, LOW);
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), contarPulsos, RISING);

  // RS485 / Modbus
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_DE, LOW);
  RS485Serial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);
  node.begin(1, RS485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // Sensores
  ds18b20.begin();
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  sht31_low_ok  = sht31_low.begin(SHT31_ADDR_LOW);
  sht31_high_ok = sht31_high.begin(SHT31_ADDR_HIGH);

  Serial.printf("[I2C] SHT31 0x44: %s\n", sht31_low_ok  ? "OK" : "NO DETECTADO");
  Serial.printf("[I2C] SHT31 0x45: %s\n", sht31_high_ok ? "OK" : "NO DETECTADO");

  // WiFi
  connectWiFi();

  Serial.println("Sistema listo. Comandos: '1' abrir valvula | '0' cerrar valvula");
}

// ===================== LOOP =====================
void loop() {
  // Reintento WiFi no bloqueante
  static unsigned long lastWifiRetry = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiRetry > 5000) {
    WiFi.reconnect();
    lastWifiRetry = millis();
  }

  // Comandos serial
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') setValvula(true);
    else if (cmd == '0') setValvula(false);
  }

  unsigned long now = millis();

  // Lectura sensores
  if (now - lastReadTime >= readInterval) {
    leerModbus();
    leerDS18B20();
    leerSHT31();
    calcularFlujo();

    Serial.printf("[DATA] soilH=%.1f soilT1=%.1f soilT2=%.2f | low: %.2f/%.1f | high: %.2f/%.1f | flowS=%.3f valve=%d\n",
      soil_humidity_1, soil_temp_1, soil_temp_2,
      air_temp_low, air_humidity_low, air_temp_high, air_humidity_high,
      flow_litros_sesion, valve_state ? 1 : 0
    );

    lastReadTime = now;
  }

  // Enviar JSON a la Raspberry
  if (now - lastSendTime >= sendInterval) {
    StaticJsonDocument<1024> doc;

    doc["device"] = "esp32-1";
    doc["soil_temp_1"] = soil_temp_1;
    doc["soil_humidity_1"] = soil_humidity_1;
    doc["soil_temp_2"] = soil_temp_2;

    doc["air_temp_low"] = air_temp_low;
    doc["air_humidity_low"] = air_humidity_low;
    doc["air_temp_high"] = air_temp_high;
    doc["air_humidity_high"] = air_humidity_high;

    doc["flow_litros_sesion"] = flow_litros_sesion;
    doc["flow_litros_total"]  = flow_litros_total;
    doc["valve_state"] = valve_state ? 1 : 0;

    doc["modbus_ok"] = modbus_ok;
    doc["ds18b20_ok"] = ds18b20_ok;
    doc["sht31_low_ok"] = sht31_low_ok;
    doc["sht31_high_ok"] = sht31_high_ok;

    String payload;
    serializeJson(doc, payload);

    bool ok = postJSON(payload);
    if (!ok) Serial.println("[POST] Fallo, reintentara luego.");

    lastSendTime = now;
  }
}
