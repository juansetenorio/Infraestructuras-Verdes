# TARS Riego — firmware ESP32-S3

Firmware modular para el conjunto completo de sensores:

- 2 × SHT31 en buses I2C independientes.
- 1 × DS18B20.
- 1 × sensor de suelo JXBS-3001-TR por RS485/Modbus RTU.
- 1 × caudalímetro FS400A.
- 1 × electroválvula controlada mediante relé o MOSFET.
- Envío periódico de atributos NGSI v2 mediante `PATCH`.
- Autenticación opcional con Keyrock.
- Dashboard web local.
- Control web para abrir/cerrar la electroválvula.
- Web OTA para subir firmware `.bin`.
- Configuración persistente en NVS.
- Total de litros persistente en NVS.
- Máquina de estados `INICIO → LECTURA → ENVIO → LECTURA`.

## Comportamiento hidráulico importante

La electroválvula arranca **cerrada**.

El caudalímetro no solo ignora los pulsos cuando la válvula está cerrada: la interrupción del GPIO 11 se desconecta físicamente mediante `detachInterrupt()`. Al abrir la válvula se vuelve a conectar mediante `attachInterrupt()`.

Por seguridad, la válvula también se cierra:

- antes de una actualización Web OTA;
- antes de un reinicio solicitado desde la configuración;
- durante el arranque del ESP32.

El total acumulado de litros no se borra al cerrar la válvula. Solo se borra usando el botón **Reiniciar total** del dashboard.

## Advertencia eléctrica

No conecte una electroválvula directamente al GPIO 12 del ESP32.

Debe utilizarse uno de estos circuitos:

- módulo de relé compatible con lógica de 3.3 V;
- MOSFET de nivel lógico;
- transistor de potencia con etapa de manejo.

La bobina requiere diodo flyback, alimentación adecuada y tierra común con el ESP32 cuando corresponda. Ver `WIRING.md`.

## Estructura

```text
TARS_Riego/
├── TARS_Riego.ino
├── AppConfig.h / AppConfig.cpp
├── WiFiManager.h / WiFiManager.cpp
├── TokenManager.h / TokenManager.cpp
├── State.h
├── StateMachine.h / StateMachine.cpp
├── Estados.h / Estados.cpp
├── SensorManager.h / SensorManager.cpp
├── ValveManager.h / ValveManager.cpp
├── PayloadBuilder.h / PayloadBuilder.cpp
├── LocalWebServer.h / LocalWebServer.cpp
├── FIWARE_PAYLOAD_EXAMPLE.json
├── WIRING.md
└── README.md
```

## Pines

| Función | GPIO |
|---|---:|
| SHT31 bus 0 SDA | 4 |
| SHT31 bus 0 SCL | 5 |
| SHT31 bus 1 SDA | 8 |
| SHT31 bus 1 SCL | 9 |
| DS18B20 OneWire | 10 |
| Caudalímetro FS400A | 11 |
| Electroválvula / relé | 12 |
| RS485 RO → RX | 39 |
| RS485 DI ← TX | 40 |
| RS485 DE/RE | 41 |

El control de la válvula está configurado como activo en `HIGH`. Si el módulo de relé es activo en `LOW`, cambie en `ValveManager.h`:

```cpp
#define VALVE_ACTIVE_LEVEL LOW
```

## Librerías

Además del core ESP32 para Arduino:

- ArduinoJson 7.x
- OneWire
- DallasTemperature

Las siguientes vienen con el core ESP32:

- WiFi
- WebServer
- ESPmDNS
- Preferences
- Update
- HTTPClient
- WiFiClientSecure
- Wire

## Uso con Arduino IDE

1. Abra la carpeta `TARS_Riego`.
2. Abra `TARS_Riego.ino`.
3. Seleccione la tarjeta ESP32-S3 correspondiente.
4. Seleccione una tabla de particiones que permita OTA. No use una partición que ocupe toda la flash con una sola aplicación.
5. Instale las librerías indicadas.
6. Compile y cargue por USB la primera vez.
7. En las siguientes actualizaciones puede exportar el binario compilado y subirlo desde la tarjeta **Web OTA**.

## Acceso local

Conectado al router, use el nombre único mostrado en el monitor serial, por ejemplo:

```text
http://tars-riego-a1b2c3.local
```

También puede usar la IP mostrada en el monitor serial.

Cuando no logra conectarse al router, publica un AP derivado del nombre:

```text
SSID de ejemplo: TARS-RIEGO-A1B2C3-SETUP
Clave: 12345678
IP: http://192.168.4.1
```

## Rutas web

| Método | Ruta | Función |
|---|---|---|
| GET | `/` | Dashboard |
| GET | `/api/data` | Datos JSON actuales |
| GET | `/data` | Alias compatible con el código original |
| POST | `/api/valve` | `action=open` o `action=close` |
| POST | `/api/reset-flow` | Reinicia litros acumulados |
| GET | `/config` | Página de configuración |
| POST | `/config` | Guarda configuración |
| POST | `/ota` | Recibe firmware `.bin` |
| GET | `/health` | Estado del servidor local |

## FIWARE

El firmware no crea contenedores, entidades, suscripciones ni modifica Orion, Keyrock, Wilma, QuantumLeap, CrateDB o Grafana.

Solamente realiza:

```text
PATCH <serverUrl>
Content-Type: application/json
Authorization: Bearer <token>
```

La entidad de `serverUrl` debe existir previamente. El valor inicial es:

```text
http://10.38.35.216:1026/v2/entities/tars-riego-01/attrs
```

Puede cambiarse desde `/config`.

### Atributos enviados

- `valveOpen`
- `flowSensorEnabled`
- `flowRate`
- `flowTotal`
- `flowFrequency`
- `flowPulses`
- `ambientTemperatureOk`
- `ambientTemperature`
- `sht31Bus0Ok`
- `sht31Bus0Temperature`
- `sht31Bus0Humidity`
- `sht31Bus1Ok`
- `sht31Bus1Temperature`
- `sht31Bus1Humidity`
- `soilSensorOk`
- `soilTemperature`
- `soilHumidity`

Cuando un sensor falla, su atributo `...Ok` se envía como `false` y sus mediciones se omiten para no reemplazar en FIWARE un dato válido por `-99`.

## Calibración del caudal

El código original utiliza:

```text
flowRate = frecuencia / 4.8
totalLiters = pulsos / 288
```

Ambas expresiones son equivalentes porque:

```text
4.8 pulsos/s por L/min × 60 s = 288 pulsos/L
```

El nuevo firmware mantiene un único parámetro: `pulsosPorLitro = 288.0`. Puede ajustarse desde `/config`.

La tasa se calcula usando el tiempo real transcurrido, no suponiendo que el ciclo duró exactamente un segundo.

## Mejoras aplicadas al código inicial

- Máquina de estados con transición diferida y sin borrar un estado durante su propio `execute()`.
- Caudal habilitado exclusivamente con válvula abierta.
- Lectura DS18B20 no bloqueante.
- CRC8 real para los dos SHT31.
- CRC16 y validación de trama para Modbus RTU.
- Temperatura de suelo interpretada como entero con signo.
- Total de caudal persistente.
- Reintento de token al recibir HTTP 401.
- Soporte HTTP y HTTPS.
- Dashboard actualizado cada segundo.
- Web OTA con validación `.bin`, progreso y cierre preventivo de válvula.

## Varios ESP de sensores en la misma red

Esta versión evita que todos los equipos publiquen `tars-riego.local`.

En el primer arranque, cada ESP32 genera un nombre único con los últimos seis
dígitos de su identificador de chip, por ejemplo:

```text
tars-riego-a1b2c3.local
TARS-RIEGO-A1B2C3-SETUP
```

El nombre se puede cambiar desde **Configuración → Identidad del dispositivo**.
Para cuatro equipos se recomienda, por ejemplo:

```text
tars-riego-01
tars-riego-02
tars-riego-03
tars-riego-04
```

El nombre elegido se guarda en NVS y controla simultáneamente:

- la dirección `http://nombre.local`;
- el SSID de respaldo `NOMBRE-SETUP`;
- el nombre visible en el dashboard.

Cambiar el nombre reinicia el ESP32. La entidad FIWARE es independiente: cada
equipo debe conservar una URL de entidad distinta en el campo `serverUrl`.

> **Migración desde la versión anterior:** después de actualizar, el antiguo
> `tars-riego.local` puede dejar de responder porque el ESP adopta un nombre
> automático único. El nuevo nombre y la IP aparecen en el monitor serial; la
> página también puede abrirse por la IP del router para asignar el nombre final.
