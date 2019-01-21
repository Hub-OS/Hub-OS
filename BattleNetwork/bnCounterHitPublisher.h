#pragma once
#pragma once

#include <list>
#include "bnCounterHitListener.h"

class Character;

class CounterHitPublisher {
  friend class CounterHitListener;

private:
  std::list<CounterHitListener*> listeners;

  void AddListener(CounterHitListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~CounterHitPublisher();

  void Broadcast(Character& victim, Character& aggressor) {
    std::list<CounterHitListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnCounter(victim, aggressor);
      iter++;
    }
  }
};