# Validación realizada

- 10/10 unidades C++ pasaron una verificación de sintaxis con `g++` y stubs de las APIs ESP32/Arduino empleadas.
- Llaves balanceadas en todos los `.ino`, `.h` y `.cpp`.
- Se verificó la existencia de:
  - `POST /api/valve`;
  - `POST /api/reset-flow`;
  - `POST /ota`;
  - `GET /api/data`;
  - payload con `valveOpen`;
  - `attachInterrupt()` al habilitar flujo;
  - `detachInterrupt()` al deshabilitar flujo.
- No se encontró lógica de batería.
- El ZIP fue creado después de las comprobaciones.

Limitación: no se ejecutó una compilación real con el core ESP32-S3 porque `arduino-cli` no está instalado en el entorno. La selección exacta de tarjeta, versión del core y tabla de particiones debe hacerse en Arduino IDE.

## Corrección de compilación 2026-07-23

Se agregó `#include <Arduino.h>` directamente en `Estados.h`. El encabezado declara métodos que reciben `const String&`, por lo que debe ser autocontenido y no depender del orden en que otros archivos incluyan Arduino.
