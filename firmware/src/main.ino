#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <ModbusMaster.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_SHT31.h>
#include <WebServer.h>   // 👈 NUEVO

// ===================== WIFI =====================
const char* WIFI_SSID = "MKSPC";
const char* WIFI_PASS = "Mkspc2308";
//const char* WIFI_SSID = "Claro_2C06BE";
//const char* WIFI_PASS = "16652524";

// ===================== SERVIDOR =====================
WebServer server(80);

// ===================== PINOUT =====================
#define I2C_SDA 8
#define I2C_SCL 9
#define MAX485_DE 16
#define TX2_PIN   17
#define RX2_PIN   18
#define ONE_WIRE_BUS 4
#define FLOW_PIN     5
#define VALVE_PIN    6

// ===================== SHT31 =====================
const uint8_t SHT31_ADDR_LOW  = 0x44;
const uint8_t SHT31_ADDR_HIGH = 0x45;

// ===================== CONFIG =====================
const float PULSOS_POR_LITRO = 288.0f;
const unsigned long readInterval = 2000;

// ===================== OBJETOS =====================
HardwareSerial RS485Serial(1);
ModbusMaster node;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
Adafruit_SHT31 sht31_low;
Adafruit_SHT31 sht31_high;

// ===================== VARIABLES =====================
float soil_temp_1 = 0, soil_humidity_1 = 0, soil_temp_2 = 0;
float air_temp_low = 0, air_humidity_low = 0;
float air_temp_high = 0, air_humidity_high = 0;

bool sht31_low_ok = false;
bool sht31_high_ok = false;

unsigned long lastReadTime = 0;

// ===================== HTML =====================
String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<style>body{font-family:Arial; text-align:center;} .box{margin:10px;padding:10px;border:1px solid #ccc;}</style>";
  html += "</head><body>";

  html += "<h2>MONITOREO ESP32</h2>";

  html += "<div class='box'><h3>SHT31 BAJO</h3>";
  html += "Temp: " + String(air_temp_low) + " C<br>";
  html += "Hum: " + String(air_humidity_low) + " %</div>";

  html += "<div class='box'><h3>SHT31 ALTO</h3>";
  html += "Temp: " + String(air_temp_high) + " C<br>";
  html += "Hum: " + String(air_humidity_high) + " %</div>";

  html += "<div class='box'><h3>SUELO</h3>";
  html += "Temp1: " + String(soil_temp_1) + "<br>";
  html += "Hum1: " + String(soil_humidity_1) + "<br>";
  html += "Temp2: " + String(soil_temp_2) + "</div>";

  html += "</body></html>";
  return html;
}

// ===================== API JSON =====================
void handleData() {
  StaticJsonDocument<512> doc;

  doc["air_temp_low"] = air_temp_low;
  doc["air_temp_high"] = air_temp_high;
  doc["air_humidity_low"] = air_humidity_low;
  doc["air_humidity_high"] = air_humidity_high;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ===================== SENSORES =====================
void leerSHT31() {

  if (!sht31_low_ok) sht31_low_ok = sht31_low.begin(SHT31_ADDR_LOW);
  if (!sht31_high_ok) sht31_high_ok = sht31_high.begin(SHT31_ADDR_HIGH);

  float t1 = sht31_low.readTemperature();
  float h1 = sht31_low.readHumidity();

  if (!isnan(t1) && !isnan(h1)) {
    air_temp_low = t1;
    air_humidity_low = h1;
  } else {
    sht31_low_ok = false;
  }

  float t2 = sht31_high.readTemperature();
  float h2 = sht31_high.readHumidity();

  if (!isnan(t2) && !isnan(h2)) {
    air_temp_high = t2;
    air_humidity_high = h2;
  } else {
    sht31_high_ok = false;
  }
}

void leerDS18B20() {
  ds18b20.requestTemperatures();
  soil_temp_2 = ds18b20.getTempCByIndex(0);
}

void leerModbus() {
  uint8_t res = node.readHoldingRegisters(2, 2);
  if (res == node.ku8MBSuccess) {
    soil_humidity_1 = node.getResponseBuffer(0) / 10.0f;
    soil_temp_1     = node.getResponseBuffer(1) / 10.0f;
  }
}

// ===================== WIFI =====================
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.println(WiFi.localIP());
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setTimeout(50);

  ds18b20.begin();

  sht31_low.begin(SHT31_ADDR_LOW);
  sht31_high.begin(SHT31_ADDR_HIGH);

  connectWiFi();

  // 🌐 RUTAS WEB
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });

  server.on("/data", handleData);

  server.begin();

  Serial.println("Servidor web iniciado");
}

// ===================== LOOP =====================
void loop() {

  server.handleClient();  // 👈 IMPORTANTE

  unsigned long now = millis();

  if (now - lastReadTime >= readInterval) {
    leerModbus();
    leerDS18B20();
    leerSHT31();
    lastReadTime = now;
  }

  /*
  // ===================== POST A RASPBERRY (DESACTIVADO) =====================
  // Puedes volverlo a activar cuando la uses

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://192.168.0.110:5000/data");
    http.addHeader("Content-Type", "application/json");

    String json = "{}";
    http.POST(json);
    http.end();
  }
  */
}
