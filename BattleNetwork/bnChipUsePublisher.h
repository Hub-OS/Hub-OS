#pragma once

#include <list>

#include "bnComponent.h"
#include "bnChip.h"
#include "bnChipUseListener.h"

class Character;

<<<<<<< HEAD
=======
/**
 * @class ChipUsePublisher
 * @author mav
 * @date 05/05/19
 * @brief Emits chip use information to all subscribers
 * @see CounterHitPublisher
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class ChipUsePublisher {
private:
  friend class ChipUseListener;

  std::list<ChipUseListener*> listeners; /*!< All subscribers */

  void AddListener(ChipUseListener* listener) {
    listeners.push_back(listener);
    std::cout << "listeners: " << listeners.size() << std::endl;
  }

public:
  virtual ~ChipUsePublisher();
<<<<<<< HEAD
  virtual void UseNextChip() = 0;

=======
  
  /**
   * @brief Must implement
   */
  virtual void UseNextChip() = 0;

 /**
  * @brief Broadcasts the chip information to all listeners
  * @param chip being used
  * @param user using the chip
  */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Broadcast(Chip& chip, Character& user) {
    std::list<ChipUseListener*>::iterator iter = listeners.begin();

    while (iter != listeners.end()) {
      (*iter)->OnChipUse(chip, user);
      iter++;
    }
  }
};