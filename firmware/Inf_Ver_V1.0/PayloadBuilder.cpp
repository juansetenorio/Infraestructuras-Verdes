#include "PayloadBuilder.h"

#include <ArduinoJson.h>
#include <math.h>

/* 
 * Función auxiliar para añadir un número al JSON con un formato específico.
 * Crea un sub-objeto con "type": "Number" y redondea el "value" a los decimales indicados.
 */
static void addNumber(JsonDocument& doc, const char* name, float value,
                      int decimals = 3) {
  // Crea un objeto dentro del documento JSON con el nombre pasado como argumento
  JsonObject attribute = doc[name].to<JsonObject>();
  attribute["type"] = "Number"; // Define el tipo estricto que pide la plataforma

  // Cálculo matemático para redondear el valor al número de decimales (por defecto 3)
  float scale = 1.0f;
  for (int i = 0; i < decimals; i++) scale *= 10.0f;
  attribute["value"] = roundf(value * scale) / scale; 
}

/* 
 * Función auxiliar para añadir un número entero (sin decimales) al JSON.
 * Aunque no la estamos usando para tus 7 variables actuales, es útil conservarla
 * por si necesitas volver a enviar pulsos o frecuencias más adelante.
 */
static void addInteger(JsonDocument& doc, const char* name, uint32_t value) {
  JsonObject attribute = doc[name].to<JsonObject>();
  attribute["type"] = "Number";
  attribute["value"] = value;
}

/* 
 * Función auxiliar para añadir un valor booleano (true/false) al JSON.
 * Igualmente, se conserva en caso de que necesites enviar estados lógicos en el futuro.
 */
static void addBoolean(JsonDocument& doc, const char* name, bool value) {
  JsonObject attribute = doc[name].to<JsonObject>();
  attribute["type"] = "Boolean";
  attribute["value"] = value;
}

/* 
 * Función principal que construye todo el paquete de datos (payload).
 * Recibe los datos de los sensores a través de la estructura 'SensorData'.
 */
String construirPayload(const SensorData& data, bool valveOpen) {
  JsonDocument doc; // Crea el documento JSON vacío en memoria

  // 1. DS18B20: Sensor de temperatura de suelo.
  // Se verifica si la lectura fue exitosa (dsOk). Si es así, se añade al JSON.
  // Se le asigna la clave "temperature" para que coincida con tu plataforma.
  if (data.dsOk) {
    addNumber(doc, "temperature", data.ambientTempC, 2); // Limitado a 2 decimales
  }

  // 2. SHT31 bus 0: Sensor posicionado a 60 cm.
  // Si la lectura es válida, se añaden temperatura y humedad con las claves específicas.
  if (data.sht0Ok) {
    addNumber(doc, "temp_60", data.sht0TempC, 2);
    addNumber(doc, "hum_60", data.sht0HumPct, 2);
  }

  // 3. SHT31 bus 1: Sensor posicionado a 15 cm.
  // Mismo proceso, mapeando a las variables de 15 cm de tu plataforma.
  if (data.sht1Ok) {
    addNumber(doc, "temp_15", data.sht1TempC, 2);
    addNumber(doc, "hum_15", data.sht1HumPct, 2);
  }

  // 4. Módulo del Sensor de suelo (temperatura y humedad).
  // Se mapea a temp_soil y hum_soil tal como marcaste en la imagen.
  if (data.soilOk) {
    addNumber(doc, "temp_soil", data.soilTempC, 2);
    addNumber(doc, "hum_soil", data.soilHumPct, 2);
  }

  // Variable final que contendrá el JSON en formato texto
  String payload;
  
  // Serializa (convierte) el objeto JSON de memoria a un String legible
  serializeJson(doc, payload); 
  
  return payload; // Devuelve el String final listo para publicarse por MQTT, HTTP, etc.
}