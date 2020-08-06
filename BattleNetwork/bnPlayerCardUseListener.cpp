#include "bnPlayerCardUseListener.h"

#include "bnFishy.h"
#include "bnTile.h"
#include "bnCannonCardAction.h"
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
#include "bnBasicSword.h"
#include "bnThunder.h"
#include "bnInvis.h"
#include "bnElecpulse.h"
#include "bnHideUntil.h"

void PlayerCardUseListener::OnCardUse(Battle::Card& card, Character& character, long long timestamp) {
  // Player charging is cancelled
  player->SetCharging(false);

  // Identify the card by the name
  std::string name = card.GetShortName();

  if (name.substr(0, 5) == "Recov") {
    auto action = new RecoverCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "CrckPanel") {
    // Crack the top, middle, and bottom row in front of player

    int dir = player->GetTeam() == Team::red ? 1 : -1;

    Battle::Tile* top = player->GetField()->GetAt(player->GetTile()->GetX() + dir, 1);
    Battle::Tile* mid = player->GetField()->GetAt(player->GetTile()->GetX() + dir, 2);
    Battle::Tile* low = player->GetField()->GetAt(player->GetTile()->GetX() + dir, 3);

    // If the tiles are valid, set their state to CRACKED
    if (top) { top->SetState(TileState::cracked); }
    if (mid) { mid->SetState(TileState::cracked); }
    if (low) { low->SetState(TileState::cracked); }

    AUDIO.Play(AudioType::PANEL_CRACK);
  }
  else if (name == "YoYo") {
    auto action = new YoYoCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "Invis") {
    // Create an invisible component. This handles the logic for timed invis
    Component* invis = new Invis(player);

    // Add the component to player
    player->RegisterComponent(invis);
  }
  else if (name.substr(0,7) == "Rflectr") {
    auto action = new ReflectCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "Fishy") {
    /**
      * Fishy is two pieces: the Fishy attack and a HideUntil component
      *
      * The fishy moves right
      *
      * HideUntil is a special component that removes entity from play
      * until a condition is met. This condition is defined in a
      * HideUntill::Callback query functor.
      *
      * In this case, we hide until the fishy is deleted whether by
      * reaching the end of the field or by a successful attack. The
      * query functor will then return true.
      *
      * When HideUntill condition is met, the component will add the entity back
      * in its original place and then removes itself from the component
      * owner
      */
    Fishy* fishy = new Fishy(player->GetField(), player->GetTeam(), 1.0);
    fishy->SetDirection(Direction::right);

    // Condition to end hide
    HideUntil::Callback until = [fishy]() { return fishy->IsDeleted(); };

    // First argument is the entity to hide
    // Second argument is the query functor
    HideUntil* fishyStatus = new HideUntil(player, until);
    player->RegisterComponent(fishyStatus);

    Battle::Tile* tile = player->GetTile();

    if (tile) {
      player->GetField()->AddEntity(*fishy, tile->GetX(), tile->GetY());
    }
  }
  else if (name == "XtrmeCnnon") {
    auto action = new CannonCardAction(player, card.GetDamage(), CannonCardAction::Type::red);
    player->QueueAction(action);
  }
  else if (name == "TwinFang") {
    auto action = new TwinFangCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "Tornado") {
    auto action = new TornadoCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "ElecSwrd") {
    auto action = new ElecSwordCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name.substr(0, 7) == "FireBrn") {
    auto type = FireBurn::Type(std::atoi(name.substr(7, 1).c_str()));
    auto action = new FireBurnCardAction(player, type, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name.substr(0, 6) == "Vulcan") {
    auto action = new VulcanCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name.size() >= 6 && name.substr(0, 6) == "Cannon") {
    auto action = new CannonCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "HiCannon") {
    auto action = new CannonCardAction(player, card.GetDamage(), CannonCardAction::Type::blue);
    player->QueueAction(action);
  }
  else if (name == "M-Cannon") {
    auto action = new CannonCardAction(player, card.GetDamage(), CannonCardAction::Type::red);
    player->QueueAction(action);
  }
  else if (name == "MiniBomb") {
    auto action = new BombCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "CrakShot") {
    auto action = new CrackShotCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "Sword") {
    auto action = new SwordCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "ElcPuls1") {
    auto action = new ElecPulseCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "LongSwrd") {
    auto action = new LongSwordCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "WideSwrd") {
    auto action = new WideSwordCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "FireSwrd") {
    auto action = new LongSwordCardAction(player, card.GetDamage());
    action->SetElement(Element::fire);
    player->QueueAction(action);
  }
  else if (name == "AirShot") {
    auto action = new AirShotCardAction(player, card.GetDamage());
    player->QueueAction(action);
  }
  else if (name == "Thunder") {
    player->QueueAction(new ThunderCardAction(player, card.GetDamage()));
  }
}