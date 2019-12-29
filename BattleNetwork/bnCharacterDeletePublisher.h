#pragma once
#pragma once

#include <list>
#include "bnCharacterDeleteListener.h"

class Character;

/**
 * @class CharacterDeletePublisher
 * @author mav
 * @date 12/28/19
 * @brief Emit a delete event. Primarily used by the Field when permanently removing Characters from play.
 */
class CharacterDeletePublisher {
  friend class CharacterDeleteListener;

private:
  std::list<CharacterDeleteListener*> listeners; /*!< List of subscriptions */

  /**
   * @brief Add a listener to subscriptions
   * @param listener
   */
  void AddListener(CharacterDeleteListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~CharacterDeletePublisher();

  /**
   * @brief Emits an event to all subscribers
   * @param pending who will be removed
   */
  void Broadcast(Character& pending) {
    std::list<CharacterDeleteListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnDeleteEvent(pending);
      iter++;
    }
  }
}; 
