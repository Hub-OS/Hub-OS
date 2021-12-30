#pragma once

#include <map>
#include <list>
#include <memory>
#include "bnCharacter.h"
#include "bnCharacterSpawnListener.h"

/**
 * @class CharacterSpawnPublisher
 * @author mav
 * @date 12/30/21
 * @brief Emit a spawn event. Primarily used by the Field when adding Characters to the field for extra special events.
 */
class CharacterSpawnPublisher {
  friend class CharacterSpawnListener;

private:
  std::list<CharacterSpawnListener*> listeners; /*!< List of subscriptions */

  /**
   * @brief Add a listener to subscriptions
   * @param listener
   */
  void AddListener(CharacterSpawnListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~CharacterSpawnPublisher();

  /**
   * @brief Emits an event to all subscribers
   * @param pending who will be removed
   */
  void Broadcast(std::shared_ptr<Character>& spawned);
}; 
