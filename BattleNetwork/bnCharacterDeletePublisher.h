#pragma once

#include <map>
#include <list>
#include "bnEntity.h"
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

  // I don't know a better way to handle this at this time
  // but entities animating their delete event are flagged as `Deleted`
  // each time the frame starts so this is broadcast every time the entity is processed
  // until is is finally removed
  // TODO: track the frame number the entity was also deleted on and skip if the frame number has passed?
  std::map<EntityID_t, bool> didOnce;

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
  void Broadcast(Character& pending);
}; 
