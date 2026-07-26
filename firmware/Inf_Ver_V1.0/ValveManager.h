#ifndef VALVE_MANAGER_H
#define VALVE_MANAGER_H

#include <Arduino.h>

// Puente H
#define VALVE_IN3_PIN 13
#define VALVE_IN4_PIN 12

// Ancho de pulso indicado por el fabricante
#define VALVE_PULSE_MS 30

class ValveManager {
 private:
  bool openState = false;

  void stopBridge();
  void pulseOpen();
  void pulseClose();

 public:
  void begin();

  bool open();
  bool close();
  bool setOpen(bool shouldOpen);

  bool isOpen() const {
    return openState;
  }
};

extern ValveManager valveManager;

#endif