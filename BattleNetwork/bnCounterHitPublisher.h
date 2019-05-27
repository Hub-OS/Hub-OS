#pragma once
#pragma once

#include <list>
#include "bnCounterHitListener.h"

class Character;

<<<<<<< HEAD
=======
/**
 * @class CounterHitPublisher
 * @author mav
 * @date 05/05/19
 * @brief Emit a counter event. Primarily used by Characters during battle.
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class CounterHitPublisher {
  friend class CounterHitListener;

private:
<<<<<<< HEAD
  std::list<CounterHitListener*> listeners;

=======
  std::list<CounterHitListener*> listeners; /*!< List of subscriptions */

  /**
   * @brief Add a listener to subscriptions
   * @param listener
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void AddListener(CounterHitListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~CounterHitPublisher();
<<<<<<< HEAD

=======
  
  /**
   * @brief Emits an event to all subscribers 
   * @param victim who was hit
   * @param aggressor who hit the victim to trigger this event
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Broadcast(Character& victim, Character& aggressor) {
    std::list<CounterHitListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnCounter(victim, aggressor);
      iter++;
    }
  }
};