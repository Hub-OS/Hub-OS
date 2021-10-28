#pragma once
#pragma once

#include <list>
#include "bnCounterHitListener.h"

class Character;

/**
 * @class CounterHitPublisher
 * @author mav
 * @date 05/05/19
 * @brief Emit a counter event. Primarily used by Characters during battle.
 */
class CounterHitPublisher {
  friend class CounterHitListener;

private:
  std::list<CounterHitListener*> listeners; /*!< List of subscriptions */

  /**
   * @brief Add a listener to subscriptions
   * @param listener
   */
  void AddListener(CounterHitListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~CounterHitPublisher();
  
  /**
   * @brief Emits an event to all subscribers 
   * @param victim who was hit
   * @param aggressor who hit the victim to trigger this event
   */
  void Broadcast(Entity& victim, Entity& aggressor) {
    std::list<CounterHitListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnCounter(victim, aggressor);
      iter++;
    }
  }
};