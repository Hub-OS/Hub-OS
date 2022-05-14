#include "bnTile.h"
#include "bnInvalidCardAction.h"
#include "bnCardBuilderTrait.h"
#include "bnCardPackageManager.h"

/***
 * Utility function to map a light weight card property object to an action from the package manager
 */

std::shared_ptr<CardAction> CardToAction(
  const Battle::Card& card, 
  std::shared_ptr<Character> character, 
  CardPackagePartitioner* partition, 
  std::optional<Battle::Card::Properties> moddedProps
) {
  if (!character) return nullptr;

  stx::result_t<PackageAddress> maybe_addr = PackageAddress::FromStr(card.GetUUID());

  if (partition && !maybe_addr.is_error()) {
    PackageAddress addr = maybe_addr.value();

    if (partition->HasPackage(addr)) {
      CardMeta& meta = partition->FindPackageByAddress(addr);
      std::unique_ptr<CardBuilderTrait> cardImpl = std::unique_ptr<CardBuilderTrait>(meta.GetData());
      const Battle::Card::Properties& props = moddedProps.has_value() ? moddedProps.value() : meta.GetCardProperties();

      std::shared_ptr<CardAction> result = cardImpl->BuildCardAction(character, props);

      if (result) {
        result->SetMetaData(props);
        return result;
      }
    }
  }

  return std::make_shared<InvalidCardAction>(character);
}