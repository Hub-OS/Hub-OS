/*! \brief Describes what happens when a card is used by any Character */

#pragma once

#include "bnAudioResourceManager.h"
#include "bnCardUseListener.h"
#include "bnCardRegistration.h"
#include "bnCard.h"

class RealtimeCardUseListener : public CardUseListener {
private:
  CardRegistration& roster;

public:
  RealtimeCardUseListener(CardRegistration& roster) : CardUseListener(), roster(roster) {}
  
  /**
   * @brief What happens when a card is used
   * @param card Card used
   * @param character Character using card
   */
  void OnCardUse(const Battle::Card& card, Character& character, long long timestamp);
};
