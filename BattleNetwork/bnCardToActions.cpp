#include "bnTile.h"
#include "bnCardPackageManager.h"
#include "bnInvalidCardAction.h"
#include "bnCardPackageManager.h"
/***
 * all of this code will be tossed out when scripting cards is complete. 
 * This is for demonstration of the engine until we have scripting done
 */

std::shared_ptr<CardAction> CardToAction(
  const Battle::Card& card, 
  std::shared_ptr<Character> character, 
  CardPackageManager* packageManager, 
  std::optional<Battle::Card::Properties> moddedProps
) {
  if (!character) return nullptr;

  if (packageManager && packageManager->HasPackage(card.GetUUID())) {
    CardMeta& meta = packageManager->FindPackageByID(card.GetUUID());
    std::unique_ptr<CardImpl> cardImpl = std::unique_ptr<CardImpl>(meta.GetData());

    const Battle::Card::Properties& props = moddedProps.has_value() ? moddedProps.value() : meta.GetCardProperties();
    std::shared_ptr<CardAction> result = cardImpl->BuildCardAction(character, props);
    result->SetMetaData(props);
    return result;
  }

  return std::make_shared<InvalidCardAction>(character);
}