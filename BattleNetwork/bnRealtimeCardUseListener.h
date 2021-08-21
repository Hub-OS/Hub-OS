/*! \brief Describes what happens when a card is used by any Character */

#pragma once

#include "bnAudioResourceManager.h"
#include "bnCardUseListener.h"
#include "bnCardPackageManager.h"
#include "bnCard.h"

class RealtimeCardActionUseListener : public CardActionUseListener {
private:
  CardPackageManager& packageManager;

public:
  RealtimeCardActionUseListener(CardPackageManager& packageManager) : CardActionUseListener(), packageManager(packageManager) {}
  
  /**
   * @brief What happens when a card is used
   * @param card Card used
   * @param character Character using card
   */
  void OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp);
};
