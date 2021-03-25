#include "bnPlayerCardUseListener.h"
#include "bnCardToActions.h"

void PlayerCardUseListener::OnCardUse(const Battle::Card& card, Character& character, long long timestamp) {
  // Player charging is cancelled
  player->Charge(false);

  if (card.IsTimeFreeze() == false) {
    if (auto* nextAction = CardToAction(card, *player)) {
      player->AddAction(CardEvent{ nextAction }, ActionOrder::voluntary);
    }
  }
}