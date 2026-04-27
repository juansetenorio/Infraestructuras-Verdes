# 🌱 Sistema IoT para Techos Verdes (ESP32-S3)

Sistema de monitoreo y control de riego para infraestructuras verdes basado en **ESP32-S3**, con envío de datos a un servidor (por ejemplo, una Raspberry Pi).

---

## 📌 Descripción

Este proyecto permite:

- Medir variables ambientales y del suelo
- Controlar una electroválvula de riego
- Calcular el consumo de agua en tiempo real
- Enviar datos vía HTTP en formato JSON

---

## ⚙️ Hardware utilizado

- ESP32-S3
- Sensor de suelo RS485 (Modbus)
- Sensor DS18B20 (temperatura)
- 2 sensores SHT31 (I2C)
- Sensor de flujo (efecto Hall)
- Electroválvula

---

## 🔌 Configuración de Pines (ESP32-S3)

### I2C
- SDA: GPIO 8  
- SCL: GPIO 9  

### RS485 (Modbus)
- DE: GPIO 16  
- TX: GPIO 17  
- RX: GPIO 18  

### Sensores
- DS18B20: GPIO 4  
- Sensor de flujo: GPIO 5  

### Actuador
- Electroválvula: GPIO 6  

---

## 🌡️ Sensores

### RS485 (Modbus)
- Humedad del suelo  
- Temperatura del suelo  

### DS18B20
- Temperatura adicional del suelo  

### SHT31 (x2)
- Dirección 0x44 → sensor zona baja  
- Dirección 0x45 → sensor zona alta  

---

## 🚿 Control de riego

La válvula se controla desde el ESP32:

- `'1'` → Abrir válvula  
- `'0'` → Cerrar válvula  

Cuando la válvula está abierta:

- Se cuentan los pulsos del sensor de flujo  
- Se calcula el volumen de agua consumido  

---

## 💧 Medición de flujo

Sensor basado en efecto Hall:
288 pulsos = 1 litro

Se calculan:

- Litros por sesión  
- Litros totales  

---

## 🌐 Conectividad

### WiFi

Configurado directamente en el código:

```cpp
const char* WIFI_SSID = "Claro_2C06BE";
const char* WIFI_PASS = "16652524";
```
## Servidor 

const char* SERVER_URL = "http://192.168.0.110:5000/data";

## 📡 Envío de datos
Método: HTTP POST
Formato: JSON
Intervalo: cada 5 segundos
```cpp
{
  "device": "esp32-1",
  "soil_temp_1": 25.4,
  "soil_humidity_1": 60.2,
  "soil_temp_2": 24.8,
  "air_temp_low": 26.1,
  "air_humidity_low": 55.3,
  "air_temp_high": 28.0,
  "air_humidity_high": 50.1,
  "flow_litros_sesion": 1.25,
  "flow_litros_total": 10.75,
  "valve_state": 1,
  "modbus_ok": true,
  "ds18b20_ok": true,
  "sht31_low_ok": true,
  "sht31_high_ok": true
}
```
## 🔁 Funcionamiento
Cada 2 segundos:
Lectura de sensores
Cálculo de flujo
Cada 5 segundos:
Envío de datos al servidor

## 🧠 Características
Manejo de errores por sensor (*_ok)
Reintento automático de conexión WiFi
Lectura de flujo mediante interrupciones
Comunicación industrial RS485 (Modbus)

## 🖥️ Monitor Serial

Ejemplo:
[DATA] soilH=60.2 soilT1=25.4 soilT2=24.80 | low: 26.10/55.3 | high: 28.00/50.1 | flowS=1.250 valve=1

