#include "bnOwnedCardsUI.h"

using std::to_string;

OwnedCardsUI::OwnedCardsUI(std::shared_ptr<Character> owner, CardPackageManager* packageManager) :
  SelectedCardsUI(owner, packageManager)
{
}

OwnedCardsUI::~OwnedCardsUI() {
}

void OwnedCardsUI::AddCards(const std::vector<Battle::Card>& cards) {
  ownedCards = cards;

  LoadCards(ownedCards);
}
