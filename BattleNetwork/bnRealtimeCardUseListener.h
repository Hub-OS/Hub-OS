/*! \brief Describes what happens when a card is used by any Character */

#pragma once

#include "bnAudioResourceManager.h"
#include "bnCardUseListener.h"
#include "bnCardPackageManager.h"
#include "bnCard.h"

class RealtimeCardActionUseListener : public CardActionUseListener {
private:
  CardPackagePartition& partition;

public:
  RealtimeCardActionUseListener(CardPackagePartition& partition) : CardActionUseListener(), partition(partition) {}
  
  /**
   * @brief What happens when a card is used
   * @param card Card used
   * @param character Character using card
   */
  void OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp);
};
