#pragma once

#include <list>

#include "bnComponent.h"
#include "bnCard.h"
#include "bnCardUseListener.h"

class Character;

/**
 * @class CardUsePublisher
 * @author mav
 * @date 05/05/19
 * @brief Emits card use information to all subscribers
 * @see CounterHitPublisher
 */
class CardUsePublisher {
private:
  friend class CardUseListener;

  std::list<CardUseListener*> listeners; /*!< All subscribers */

  void AddListener(CardUseListener* listener);

public:
  virtual ~CardUsePublisher();
  
  /**
   * @brief Must implement
   */
  virtual const bool UseNextCard() = 0;

 /**
  * @brief Broadcasts the card information to all listeners
  * @param card being used
  * @param user using the card
  */
  void Broadcast(Battle::Card& card, Character& user, uint64_t timestamp = 0);

  void DropSubscribers();
};