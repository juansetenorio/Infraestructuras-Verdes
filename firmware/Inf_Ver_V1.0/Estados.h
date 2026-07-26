#ifndef ESTADOS_H
#define ESTADOS_H

#include <Arduino.h>

#include "State.h"

class EstadoINICIO : public State {
 private:
  bool executed = false;

 public:
  void onEnter() override;
  void execute() override;
  void onExit() override;
  const char* getName() const override { return "INICIO"; }
};

class EstadoLECTURA : public State {
 public:
  void onEnter() override;
  void execute() override;
  void onExit() override;
  const char* getName() const override { return "LECTURA"; }
};

class EstadoENVIO : public State {
 private:
  bool executed = false;

  int sendPayload(const String& payload);
  int sendPayloadOnce(const String& payload);

 public:
  void onEnter() override;
  void execute() override;
  void onExit() override;
  const char* getName() const override { return "ENVIO"; }
};

#endif
