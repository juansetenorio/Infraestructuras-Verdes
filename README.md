# 🌱 Sistema de Monitoreo para Techos Verdes

Proyecto desarrollado por el grupo **AGE-VITAL** enfocado en el monitoreo y control automático de infraestructuras verdes, específicamente **techos verdes**.

---

## 📌 Descripción

Este sistema permite:

- Monitorear variables ambientales (temperatura y humedad)
- Medir condiciones del suelo
- Controlar el riego automáticamente mediante una electroválvula
- Enviar datos a un servidor para análisis en tiempo real

Todo el sistema está basado en un **ESP32 TTGO** con conectividad WiFi.

---

## 🎯 Objetivos

- Medición de temperatura y humedad ambiental  
- Medición de humedad y temperatura del suelo  
- Control automático del flujo de agua  
- Envío de datos a un servidor IoT  

---

## 🧠 Arquitectura del Sistema

El sistema integra:

- Microcontrolador **ESP32 TTGO**
- Sensores:
  - SHT31 (temperatura y humedad ambiente)
  - DS18B20 (temperatura)
  - Sensor de suelo RS485 (Modbus)
  - Sensor de flujo (efecto Hall)
- Pantalla TFT
- Electroválvula
- Comunicación WiFi

---

## 🔌 Asignación de Pines

### 📺 Pantalla TFT
- CS: GPIO 5  
- RST: GPIO 23  
- DC: GPIO 16  
- MOSI: GPIO 19  
- SCLK: GPIO 18  
- BL: GPIO 4  

### 🌱 Sensor RS485 (suelo)
- DE/RE: GPIO 25  
- RX: GPIO 33  
- TX: GPIO 32  

### 🌡️ Sensor DS18B20
- DATA: GPIO 17  

### 💧 Sensor SHT31 (I2C)
- SDA: GPIO 21  
- SCL: GPIO 22  

### 🚿 Sensor de flujo
- Señal: GPIO 2  

### 🔄 Electroválvula
- Control: GPIO 26  

---

## 🔍 Sensores utilizados

### SHT31
- Temperatura del aire  
- Humedad relativa  
- Comunicación I2C  

### DS18B20
- Temperatura  
- Comunicación OneWire  

### Sensor de suelo RS485
- Temperatura del suelo  
- Humedad del suelo  
- Comunicación Modbus RTU  

### Sensor de flujo
- Basado en efecto Hall  
- Relación: **288 pulsos = 1 litro**  

---

## 🚰 Control de riego

- La electroválvula se controla desde el ESP32  
- Cuando se activa:
  - Se inicia el conteo de flujo  
  - Se calcula el volumen de agua utilizado  

---

## 🌐 Comunicación

El sistema envía datos en formato **JSON** mediante HTTP a un servidor:

- Orion Context Broker  
- Protocolo REST  

---

## 🔁 Funcionamiento

El sistema opera en ciclos:

- ⏱️ Cada **2 segundos** → Lectura de sensores  
- 📺 Cada **5 segundos** → Actualización de pantalla  
- 🌐 Cada **10 segundos** → Envío de datos al servidor  

---

## 🖥️ Interacción

Control manual por puerto serial:

- `'1'` → Abrir válvula  
- `'0'` → Cerrar válvula  

---

## 📊 Datos enviados

Se transmiten variables como:

- Temperatura del suelo  
- Humedad del suelo  
- Temperatura del aire  
- Humedad del aire  
- Flujo de agua (sesión y total)  
- Estado de la válvula  

---

## 🚀 Tecnologías utilizadas

- ESP32 (Arduino framework)  
- WiFi  
- HTTPClient  
- ArduinoJson  
- ModbusMaster  
- Adafruit Libraries  

---

## 📈 Aplicaciones

- Agricultura urbana  
- Sistemas de riego inteligentes  
- Proyectos IoT ambientales  
- Sostenibilidad y ciudades inteligentes  

---

## ✅ Conclusión

Este sistema permite:

- Monitoreo en tiempo real  
- Optimización del uso del agua  
- Integración con plataformas IoT  

Convirtiéndose en una solución eficiente y escalable para infraestructuras verdes.
