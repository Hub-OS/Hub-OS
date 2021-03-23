#pragma once
#include "bnCard.h"

class CardUsePublisher;
class Character;

/**
 * @class CardUseListener
 * @author mav
 * @date 05/05/19
 * @brief Listens for card use events emitted by CardUsePublishers
 */
class CardUseListener {
public:
  CardUseListener() = default;
  ~CardUseListener() = default;

  CardUseListener(const CardUseListener& rhs) = delete;
  CardUseListener(CardUseListener&& rhs) = delete;

  /**
   * @brief What happens when we recieve the card event
   * @param card
   * @param user
   */
  virtual void OnCardUse(const Battle::Card& card, Character& user, long long timestamp) = 0;
  
  /**
   * @brief Subscribe this listener to a publisher objects
   * @param publisher
   */
  void Subscribe(CardUsePublisher& publisher);
};