#include "bnTile.h"
#include "bnCardPackageManager.h"
#include "bnInvalidCardAction.h"
#include "bnCardPackageManager.h"
/***
 * all of this code will be tossed out when scripting cards is complete. 
 * This is for demonstration of the engine until we have scripting done
 */

std::shared_ptr<CardAction> CardToAction(const Battle::Card& card, std::shared_ptr<Character> character, CardPackageManager* packageManager) {
  if (!character) return nullptr;

  if (packageManager && packageManager->HasPackage(card.GetUUID())) {
    auto& meta = packageManager->FindPackageByID(card.GetUUID());
    auto cardImpl = std::unique_ptr<CardImpl>(meta.GetData());
    return cardImpl->BuildCardAction(character, meta.GetCardProperties());
  }

  return std::make_shared<InvalidCardAction>(character);
}