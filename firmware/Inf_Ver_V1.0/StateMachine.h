#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

#include "State.h"

struct MachineClocks {
  unsigned long now = 0;
  unsigned long nextSend = 0;
};

struct SendStatus {
  bool lastSendOk = false;
  int lastHttpCode = 0;
  unsigned long lastSendAt = 0;
  String lastMessage = "Aún no se han enviado datos";
};

class StateMachine {
 private:
  State* currentState = nullptr;
  State* pendingState = nullptr;

  void applyPendingState();

 public:
  MachineClocks clocks;
  SendStatus sendStatus;

  StateMachine() = default;
  ~StateMachine();

  void begin(State* initialState);
  void update();
  void requestState(State* newState);

  const char* getCurrentStateName() const;
};

extern StateMachine stateMachine;

#endif
