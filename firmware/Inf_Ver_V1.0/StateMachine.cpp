#include "StateMachine.h"

StateMachine::~StateMachine() {
  if (currentState) {
    currentState->onExit();
    delete currentState;
  }
  if (pendingState) delete pendingState;
}

void StateMachine::begin(State* initialState) {
  if (!initialState) return;

  currentState = initialState;
  currentState->setStateMachine(this);
  currentState->onEnter();

  Serial.printf("[StateMachine] Estado inicial: %s\n",
                currentState->getName());
}

void StateMachine::update() {
  clocks.now = millis();

  if (currentState) {
    currentState->execute();
  }

  // La transición se aplica después de que execute() termina.
  // Esto evita borrar el estado mientras aún ejecuta su propio método.
  applyPendingState();
}

void StateMachine::requestState(State* newState) {
  if (!newState) return;

  if (pendingState) {
    delete pendingState;
  }
  pendingState = newState;
}

void StateMachine::applyPendingState() {
  if (!pendingState) return;

  Serial.printf("[StateMachine] %s -> %s\n",
                currentState ? currentState->getName() : "NULL",
                pendingState->getName());

  if (currentState) {
    currentState->onExit();
    delete currentState;
  }

  currentState = pendingState;
  pendingState = nullptr;
  currentState->setStateMachine(this);
  currentState->onEnter();
}

const char* StateMachine::getCurrentStateName() const {
  return currentState ? currentState->getName() : "NULL";
}
