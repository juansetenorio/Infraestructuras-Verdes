# Cableado y etapa de potencia

## Sensores

### SHT31 bus 0

- SDA → GPIO 4
- SCL → GPIO 5
- VCC y GND según el módulo
- Dirección I2C: `0x44`

### SHT31 bus 1

- SDA → GPIO 8
- SCL → GPIO 9
- VCC y GND según el módulo
- Dirección I2C: `0x44`

Se usan buses separados, por eso ambos sensores pueden conservar la misma dirección.

### DS18B20

- DATA → GPIO 10
- Resistencia pull-up típica de 4.7 kΩ entre DATA y 3.3 V
- VCC y GND según el encapsulado

### FS400A

- Señal → GPIO 11
- Verifique el nivel lógico de la salida.
- Si la salida supera 3.3 V, use divisor, transistor u optoacoplador.

### JXBS-3001-TR RS485

ESP32-S3 ↔ transceptor RS485:

- RO → GPIO 39
- DI → GPIO 40
- DE y RE unidos → GPIO 41
- A/B → sensor de suelo
- Dirección Modbus: `0x01`
- Velocidad: 9600, 8N1

## Electroválvula

GPIO 12 es únicamente una señal de control.

### Opción MOSFET

- GPIO 12 → resistencia de compuerta → gate del MOSFET.
- Resistencia pull-down en gate.
- Source → GND.
- Drain → terminal negativo de la electroválvula.
- Terminal positivo de la electroválvula → fuente externa.
- Diodo flyback en paralelo con la bobina.
- Tierra de la fuente y ESP32 en común.

### Opción relé

- GPIO 12 → entrada lógica del módulo.
- Use un módulo cuya entrada reconozca 3.3 V.
- La fuente de la válvula pasa por COM/NO.
- Mantenga aislamiento y capacidad de corriente adecuados.

Si el relé se activa con nivel bajo, cambie:

```cpp
#define VALVE_ACTIVE_LEVEL LOW
```

en `ValveManager.h`.

## Estado seguro

Con `NO` en el relé, una pérdida de energía deja la electroválvula cerrada, que es la configuración recomendada.
