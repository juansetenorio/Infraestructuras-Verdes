#ifndef STATE_H
#define STATE_H

class StateMachine;

class State {
 protected:
  StateMachine* statemachine = nullptr;

 public:
  virtual ~State() = default;

  void setStateMachine(StateMachine* sm) { statemachine = sm; }

  virtual void onEnter() = 0;
  virtual void execute() = 0;
  virtual void onExit() = 0;
  virtual const char* getName() const = 0;
};

#endif
