#pragma once

#include <list>

#include "bnComponent.h"
#include "bnChip.h"
#include "bnChipUseListener.h"

class Character;

/**
 * @class ChipUsePublisher
 * @author mav
 * @date 05/05/19
 * @file bnChipUsePublisher.h
 * @brief Emits chip use information to all subscribers
 * @see CounterHitPublisher
 */
class ChipUsePublisher {
private:
  friend class ChipUseListener;

  std::list<ChipUseListener*> listeners; /*!< All subscribers */

  void AddListener(ChipUseListener* listener) {
    listeners.push_back(listener);
  }

public:
  virtual ~ChipUsePublisher();
  
  /**
   * @brief Must implement
   */
  virtual void UseNextChip() = 0;

 /**
  * @brief Broadcasts the chip information to all listeners
  * @param chip being used
  * @param user using the chip
  */
  void Broadcast(Chip& chip, Character& user) {
    std::list<ChipUseListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnChipUse(chip, user);
      iter++;
    }
  }
};