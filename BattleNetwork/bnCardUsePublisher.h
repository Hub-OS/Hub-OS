#pragma once

#include <list>
#include <optional>

#include "bnComponent.h"
#include "bnCard.h"

class Character;
class CardUseListener;

/**
 * @class CardUsePublisher
 * @author mav
 * @date 05/05/19
 * @brief Emits card use information to all subscribers
 * @see CardUsePublisher
 */
class CardUsePublisher {
private:
  friend class CardUseListener;

  std::list<CardUseListener*> listeners; /*!< All subscribers */

  void AddListener(CardUseListener* listener);

protected:
  /**
 * @brief Broadcasts the card information to all listeners
 * @param card being used
 * @param user using the card
 */
  void Broadcast(const Battle::Card& card, Character& user, uint64_t timestamp = 0);

public:
  virtual ~CardUsePublisher();
  
  /**
   * @brief Must implement
   */
  virtual void UseNextCard() = 0;

  void DropSubscribers();
};