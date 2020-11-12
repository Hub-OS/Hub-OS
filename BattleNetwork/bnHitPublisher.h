#pragma once

#include <list>
#include "bnHitPublisher.h"
#include "bnHitProperties.h"

class Character;
class HitListener;

/**
 * @class HitPublisher
 * @author mav
 * @date 11/11/2020
 * @brief Emit a hit event. Primarily used by Characters during battle.
 */
class HitPublisher {
  friend class HitListener;

private:
  std::list<HitListener*> listeners; /*!< List of subscriptions */

  /**
   * @brief Add a listener to subscriptions
   * @param listener
   */
  void AddListener(HitListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~HitPublisher();

  /**
   * @brief Emits an event to all subscribers
   * @param victim who was hit
   * @param aggressor who hit the victim to trigger this event
   * @param props the hitbox properties used to hit the victim with
   */
  void Broadcast(Character& victim, const Hit::Properties& props);
};