# Revisión del código monolítico original

## Aspectos correctos

- Separación de los dos SHT31 usando buses I2C distintos.
- Uso de interrupción para el FS400A.
- Verificación CRC16 de la respuesta Modbus.
- Control DE/RE correcto para el transceptor RS485.
- Uso de `WebServer` y endpoint JSON para refrescar el dashboard.
- Conversión coherente del caudal: 4.8 Hz por L/min y 288 pulsos por litro.

## Ajustes realizados

### 1. El caudalímetro permanecía siempre habilitado

En el código original, `attachInterrupt()` se ejecutaba permanentemente desde `setup()`. Ahora:

- la válvula inicia cerrada;
- la interrupción del caudalímetro no se conecta durante el arranque;
- `attachInterrupt()` se llama al abrir la válvula;
- `detachInterrupt()` se llama al cerrarla;
- la ISR tiene una segunda validación mediante `isrFlowEnabled`.

### 2. Cálculo de caudal dependiente de un segundo exacto

El código original consideraba que la ventana siempre era exactamente de 1000 ms. El nuevo cálculo usa el tiempo real transcurrido:

```text
frecuencia = pulsos × 1000 / milisegundos_transcurridos
caudal = frecuencia × 60 / pulsos_por_litro
```

### 3. Lectura DS18B20 no realmente asíncrona

Con `setWaitForConversion(false)`, no se debe solicitar la conversión y leer inmediatamente. El nuevo firmware:

1. solicita la conversión;
2. continúa atendiendo web, WiFi y caudal;
3. lee el resultado después de 750 ms.

### 4. CRC del SHT31 ignorado

El código original recibía los bytes CRC, pero no los verificaba. Se agregó CRC8 con polinomio `0x31` para temperatura y humedad en ambos buses.

### 5. Temperatura de suelo con signo

La temperatura Modbus se interpreta ahora como `int16_t`, permitiendo valores bajo cero.

### 6. Arquitectura

El programa se dividió en:

- configuración;
- WiFi;
- token;
- sensores;
- electroválvula;
- payload;
- servidor local;
- máquina de estados;
- estados de lectura y envío.

### 7. Máquina de estados segura

Las transiciones se aplican después de finalizar `execute()`. Esto evita eliminar el objeto del estado mientras todavía está ejecutando uno de sus métodos.

### 8. Persistencia

- configuración y credenciales en NVS;
- total de pulsos del caudalímetro en NVS;
- guardado periódico y al cerrar la válvula.

### 9. Web local

Se agregaron:

- apertura/cierre de válvula;
- bloqueo visible del caudal;
- estado del último envío FIWARE;
- configuración;
- reinicio de litros;
- Web OTA con barra de progreso.

### 10. Seguridad operativa

La válvula se cierra antes de:

- Web OTA;
- reinicio por cambios de red;
- reinicio normal del firmware.
