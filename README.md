# 🌱 Sistema IoT para Infraestructuras Verdes (ESP32-S3)

Sistema de monitoreo para celdas de evaluación de infraestructuras verdes basado en **ESP32-S3**. El sistema recopila datos ambientales, temperatura y humedad del sustrato, y la percolación de agua, enviando los datos empaquetados en JSON a un servidor local o en la nube.

---

## 📌 Descripción

Este proyecto permite:

- Monitorear el microclima a diferentes alturas (15 cm y 60 cm) sobre el sustrato.
- Medir la temperatura y humedad interna del suelo mediante sensores industriales y sondas sumergibles.
- Cuantificar el volumen de agua filtrada (percolación) utilizando un pluviómetro de balancín.
- Enviar todos los datos vía HTTP en formato JSON.

---

## ⚙️ Hardware utilizado

- ESP32-S3 (Placa de control en caja de paso)
- 1x Sensor de suelo JXBS-3001-TR (Temperatura y Humedad vía RS485/Modbus)
- 1x Sensor DS18B20 (Temperatura profunda del suelo vía OneWire)
- 2x Sensores SHT31 (Temperatura y Humedad del aire vía I2C)
- 1x Pluviómetro de balancín (Medición de flujo por interrupción de pulsos)

---

## 🔌 Configuración de Pines (ESP32-S3)

### I2C (Sensores SHT31)
- **Bus 0 (15 cm):** SDA: GPIO 4 | SCL: GPIO 5  
- **Bus 1 (60 cm):** SDA: GPIO 8 | SCL: GPIO 9  

### RS485 (Modbus - JXBS-3001)
- **DE/RE (Control):** GPIO 16  
- **TX (DI):** GPIO 17  
- **RX (RO):** GPIO 18  

### Sensores Adicionales
- **DS18B20 (OneWire):** GPIO 10  
- **Pluviómetro (Pulsos):** GPIO 11  

---

## 🌡️ Direccionamiento de Sensores

### RS485 (Modbus)
- Dirección del esclavo: `0x01` a `9600 bps`.

### SHT31 (x2)
- Ambos sensores utilizan la dirección `0x44`. No hay conflicto porque están separados físicamente en el **Bus 0** y el **Bus 1** del I2C de la ESP32-S3.

---

## 💧 Medición de Percolación (Pluviómetro)

El sistema lee los pulsos generados por el balancín del pluviómetro ubicado en el drenaje de la celda.
- Se procesan los pulsos mediante interrupciones en el GPIO 11.
- Se calcula el volumen de agua filtrada (litros o milímetros) dependiendo del factor de conversión mecánico del balancín.

---

## 🌐 Conectividad

### WiFi

Configurado directamente en el código:

```cpp
const char* WIFI_SSID = "Tu_Red_WiFi";
const char* WIFI_PASS = "Tu_Contraseña";
```

### Servidor 

```cpp
const char* SERVER_URL = "[http://192.168.0.110:5000/data](http://192.168.0.110:5000/data)";
```

## 📡 Envío de datos

- **Método:** HTTP POST
- **Formato:** JSON
- **Estructura del Payload:**

```json
{
  "temperature": 24.5,
  "temp_60": 24.8,
  "hum_60": 55.3,
  "temp_15": 24.2,
  "hum_15": 52.1,
  "temp_soil": 18.7,
  "hum_soil": 40.5
}
```
*(Nota: Las variables de flujo y métricas internas se gestionan en memoria y pueden habilitarse en el constructor JSON si el servidor lo requiere).*

---

## 🔁 Funcionamiento

1. **Lectura cíclica:** La ESP32-S3 interroga los sensores I2C, OneWire y RS485.
2. **Conteo en segundo plano:** Las interrupciones cuentan los vuelcos del pluviómetro en tiempo real.
3. **Empaquetado y Envío:** La librería `ArduinoJson` construye el payload y se envía al servidor mediante una petición HTTP POST.

## 🧠 Características Destacadas
- Redundancia I2C mediante la instanciación de dos buses independientes.
- Manejo de errores de lectura: Si un sensor se desconecta, su bandera interna (`dsOk`, `sht0Ok`, etc.) evita enviar basura al JSON.
- Reintento automático de conexión WiFi.
