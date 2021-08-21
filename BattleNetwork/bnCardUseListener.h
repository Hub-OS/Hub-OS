#pragma once
#include <memory>
#include "bnCardAction.h"

class CardActionUsePublisher;

/**
 * @class CardActionUseListener
 * @author mav
 * @date 05/05/19
 * @brief Listens for card use events emitted by CardActionUsePublishers
 */
class CardActionUseListener {
public:
  CardActionUseListener() = default;
  ~CardActionUseListener() = default;

  CardActionUseListener(const CardActionUseListener& rhs) = delete;
  CardActionUseListener(CardActionUseListener&& rhs) = delete;

  /**
   * @brief What happens when we recieve the card event
   * @param card
   * @param user
   */
  virtual void OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp) = 0;
  
  /**
   * @brief Subscribe this listener to a publisher objects
   * @param publisher
   */
  void Subscribe(CardActionUsePublisher& publisher);
};