#include "bnFishy.h"
#include "bnTile.h"
#include "bnAura.h"
#include "bnCardPackageManager.h"
#include "bnWindRackCardAction.h"
#include "bnAirHockeyCardAction.h"
#include "bnHubBatchCardAction.h"
#include "bnMachGunCardAction.h"
#include "bnCannonCardAction.h"
#include "bnZetaCannonCardAction.h"
#include "bnAirShotCardAction.h"
#include "bnTwinFangCardAction.h"
#include "bnTornadoCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnElecSwordCardAction.h"
#include "bnSwordCardAction.h"
#include "bnWideSwordCardAction.h"
#include "bnLongSwordCardAction.h"
#include "bnVulcanCardAction.h"
#include "bnReflectCardAction.h"
#include "bnYoYoCardAction.h"
#include "bnBombCardAction.h"
#include "bnCrackShotCardAction.h"
#include "bnRecoverCardAction.h"
#include "bnThunderCardAction.h"
#include "bnElecPulseCardAction.h"
#include "bnDarkTornadoCardAction.h"
#include "bnRollCardAction.h"
#include "bnProtoManCardAction.h"
#include "bnCubeCardAction.h"
#include "bnAuraCardAction.h"
#include "bnAntiDmgCardAction.h"
#include "bnAreaGrabCardAction.h"
#include "bnInvisCardAction.h"
#include "bnInvalidCardAction.h"
#include "bnCardPackageManager.h"
/***
 * all of this code will be tossed out when scripting cards is complete. 
 * This is for demonstration of the engine until we have scripting done
 */

std::shared_ptr<CardAction> CardToAction(const Battle::Card& card, std::shared_ptr<Character> character, CardPackageManager* packageManager) {
  if (!character) return nullptr;

  // Identify the card by the name
  std::string name = card.GetShortName();
  std::shared_ptr<CardAction> next{nullptr};

  if (packageManager && packageManager->HasPackage(card.GetUUID())) {
    auto& meta = packageManager->FindPackageByID(card.GetUUID());
    return meta.GetData()->BuildCardAction(character, meta.GetCardProperties());
  }
  
  if (name.substr(0, 4) == "Atk+") {
    next = std::make_shared<InvalidCardAction>(character);
  }else if (name.substr(0, 5) == "Recov") {
    next = std::make_shared<RecoverCardAction>(character, card.GetDamage());
  }
  else if (name == "CrckPanel") {
    // Crack the top, middle, and bottom row in front of player

    int dir = character->GetTeam() == Team::red ? 1 : -1;

    Battle::Tile* top = character->GetField()->GetAt(character->GetTile()->GetX() + dir, 1);
    Battle::Tile* mid = character->GetField()->GetAt(character->GetTile()->GetX() + dir, 2);
    Battle::Tile* low = character->GetField()->GetAt(character->GetTile()->GetX() + dir, 3);

    // If the tiles are valid, set their state to CRACKED
    if (top) { top->SetState(TileState::cracked); }
    if (mid) { mid->SetState(TileState::cracked); }
    if (low) { low->SetState(TileState::cracked); }

    character->Audio().Play(AudioType::PANEL_CRACK);
  }
  else if (name == "YoYo") {
    next = std::make_shared<YoYoCardAction>(character, card.GetDamage());
  }
  else if (name == "Invis") {
    next = std::make_shared<InvisCardAction>(character);
  }
  else if (name.substr(0, 7) == "Rflectr") {
    next = std::make_shared<ReflectCardAction>(character, card.GetDamage(), ReflectShield::Type::yellow);
  }
  else if (name == "Zeta Cannon 1") {
    next = std::make_shared<ZetaCannonCardAction>(character, card.GetDamage());
  }
  else if (name == "TwinFang") {
    next = std::make_shared<TwinFangCardAction>(character, card.GetDamage());
  }
  else if (name == "Tornado") {
    next = std::make_shared<TornadoCardAction>(character, card.GetDamage());
  }
  else if (name == "DarkTorn") {
    next = std::make_shared<DarkTornadoCardAction>(character, card.GetDamage());
  }
  else if (name == "ElecSwrd") {
    next = std::make_shared<ElecSwordCardAction>(character, card.GetDamage());
  }
  else if (name.substr(0, 7) == "FireBrn") {
    auto type = FireBurn::Type(std::atoi(name.substr(7, 1).c_str()));
    next = std::make_shared<FireBurnCardAction>(character, type, card.GetDamage());
  }
  else if (name.substr(0, 6) == "Vulcan") {
    next = std::make_shared<VulcanCardAction>(character, card.GetDamage());
  }
  else if (name.size() >= 6 && name.substr(0, 6) == "Cannon") {
    next = std::make_shared<CannonCardAction>(character, CannonCardAction::Type::green, card.GetDamage());
  }
  else if (name == "HiCannon") {
    next = std::make_shared<CannonCardAction>(character, CannonCardAction::Type::blue, card.GetDamage());
  }
  else if (name == "M-Cannon") {
    next = std::make_shared<CannonCardAction>(character, CannonCardAction::Type::red, card.GetDamage());
  }
  else if (name == "MiniBomb") {
    next = std::make_shared<BombCardAction>(character, card.GetDamage());
  }
  else if (name == "CrakShot") {
    next = std::make_shared<CrackShotCardAction>(character, card.GetDamage());
  }
  else if (name == "Sword") {
    next = std::make_shared<SwordCardAction>(character, card.GetDamage());
  }
  else if (name == "ElcPuls1") {
    next = std::make_shared<ElecPulseCardAction>(character, card.GetDamage());
  }
  else if (name == "LongSwrd") {
    next = std::make_shared<LongSwordCardAction>(character, card.GetDamage());
  }
  else if (name == "WideSwrd") {
    next = std::make_shared<WideSwordCardAction>(character, card.GetDamage());
  }
  else if (name == "FireSwrd") {
    auto action = std::make_shared<LongSwordCardAction>(character, card.GetDamage());
    action->SetElement(Element::fire);
    next = action;
  }
  else if (name == "AirShot") {
    next = std::make_shared<AirShotCardAction>(character, card.GetDamage());
  }
  else if (name == "Thunder") {
    next = std::make_shared<ThunderCardAction>(character, card.GetDamage());
  }
  else if (name.substr(0, 4) == "Roll") {
    next = std::make_shared<RollCardAction>(character, card.GetDamage());
  }
  else if (name == "ProtoMan") {
    next = std::make_shared<ProtoManCardAction>(character, card.GetDamage());
  }
  else if (name == "RockCube") {
    next = std::make_shared<CubeCardAction>(character);
  }
  else if (name == "AreaGrab") {
    next = std::make_shared<AreaGrabCardAction>(character, 10);
  }
  else if (name == "AntiDmg") {
    next = std::make_shared<AntiDmgCardAction>(character, card.GetDamage());
  }
  else if (name == "Barrier") {
    next = std::make_shared<AuraCardAction>(character, Aura::Type::BARRIER_10);
  }
  else if (name.substr(0, 7) == "MachGun") {
    next = std::make_shared<MachGunCardAction>(character, card.GetDamage());
  }
  else if (name == "HubBatch") {
    next = std::make_shared<HubBatchCardAction>(character);
  }
  else if (name == "AirHocky") {
    next = std::make_shared<AirHockeyCardAction>(character, card.GetDamage());
  }
  else if (name == "WindRack") {
    next = std::make_shared<WindRackCardAction>(character, card.GetDamage());
  }

  return next;
}