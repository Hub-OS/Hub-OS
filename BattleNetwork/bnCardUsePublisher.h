#pragma once

#include <vector>
#include <memory>
#include "bnCardAction.h"

class CardActionUseListener;

/**
 * @class CardActionUsePublisher
 * @author mav
 * @date 05/05/19
 * @brief Emits card use information to all subscribers
 * @see CardActionUsePublisher
 */
class CardActionUsePublisher {
private:
  friend class CardActionUseListener;

  std::vector<CardActionUseListener*> listeners; /*!< All subscribers */

  void AddListener(CardActionUseListener* listener);

protected:
  /**
 * @brief Broadcasts the card information to all listeners
 * @param card being used
 * @param user using the card
 */
  void Broadcast(std::shared_ptr<CardAction> action, uint64_t timestamp = 0);

public:
  virtual ~CardActionUsePublisher();

  void DropSubscribers();
};