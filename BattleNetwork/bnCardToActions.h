#pragma once
#include "bnFishy.h"
#include "bnTile.h"
#include "bnAura.h"
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
#include "bnBasicSword.h"
#include "bnThunder.h"
#include "bnInvis.h"
#include "bnElecpulse.h"
#include "bnParticlePoof.h"
#include "bnHideUntil.h"

/***
 * all of this code will be tossed out when scripting cards is complete. 
 * This is for demonstration of the engine until we have scripting done
 */

static CardAction* CardToAction(const Battle::Card& card, Character& character) {
      // Identify the card by the name
  std::string name = card.GetShortName();
  CardAction* next{nullptr};
  
  if (name.substr(0, 4) == "Atk+") {
    Battle::Tile* tile = character.GetTile();
    ParticlePoof* poof = new ParticlePoof();
    poof->SetHeight(character.GetHeight());
    poof->SetLayer(-100); // in front of player and player widgets

    character.GetField()->AddEntity(*poof, *tile);
  }else if (name.substr(0, 5) == "Recov") {
    next = new RecoverCardAction(character, card.GetDamage());
  }
  else if (name == "CrckPanel") {
    // Crack the top, middle, and bottom row in front of player

    int dir = character.GetTeam() == Team::red ? 1 : -1;

    Battle::Tile* top = character.GetField()->GetAt(character.GetTile()->GetX() + dir, 1);
    Battle::Tile* mid = character.GetField()->GetAt(character.GetTile()->GetX() + dir, 2);
    Battle::Tile* low = character.GetField()->GetAt(character.GetTile()->GetX() + dir, 3);

    // If the tiles are valid, set their state to CRACKED
    if (top) { top->SetState(TileState::cracked); }
    if (mid) { mid->SetState(TileState::cracked); }
    if (low) { low->SetState(TileState::cracked); }

    character.Audio().Play(AudioType::PANEL_CRACK);
  }
  else if (name == "YoYo") {
     next = new YoYoCardAction(character, card.GetDamage());
  }
  else if (name == "Invis") {
    // Create an invisible component. This handles the logic for timed invis
    Component* invis = new Invis(&character);

    // Add the component to player
    character.RegisterComponent(invis);
  }
  else if (name.substr(0, 7) == "Rflectr") {
    next = new ReflectCardAction(character, card.GetDamage(), ReflectShield::Type::yellow);
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
    Fishy* fishy = new Fishy(character.GetTeam(), 1.0);
    fishy->SetDirection(Direction::right);

    // Condition to end hide
    HideUntil::Callback until = [fishy]() { return fishy->IsDeleted(); };

    // First argument is the entity to hide
    // Second argument is the query functor
    HideUntil* fishyStatus = new HideUntil(&character, until);
    character.RegisterComponent(fishyStatus);

    Battle::Tile* tile = character.GetTile();

    if (tile) {
      character.GetField()->AddEntity(*fishy, tile->GetX(), tile->GetY());
    }
  }
  else if (name == "Zeta Cannon 1") {
    next = new ZetaCannonCardAction(character, card.GetDamage());
  }
  else if (name == "TwinFang") {
    next = new TwinFangCardAction(character, card.GetDamage());
  }
  else if (name == "Tornado") {
    next = new TornadoCardAction(character, card.GetDamage());
  }
  else if (name == "DarkTorn") {
    next = new DarkTornadoCardAction(character, card.GetDamage());
  }
  else if (name == "ElecSwrd") {
    next = new ElecSwordCardAction(character, card.GetDamage());
  }
  else if (name.substr(0, 7) == "FireBrn") {
    auto type = FireBurn::Type(std::atoi(name.substr(7, 1).c_str()));
    next = new FireBurnCardAction(character, type, card.GetDamage());
  }
  else if (name.substr(0, 6) == "Vulcan") {
    next = new VulcanCardAction(character, card.GetDamage());
  }
  else if (name.size() >= 6 && name.substr(0, 6) == "Cannon") {
    next = new CannonCardAction(character, CannonCardAction::Type::green, card.GetDamage());
  }
  else if (name == "HiCannon") {
    next = new CannonCardAction(character, CannonCardAction::Type::blue, card.GetDamage());
  }
  else if (name == "M-Cannon") {
    next = new CannonCardAction(character, CannonCardAction::Type::red, card.GetDamage());
  }
  else if (name == "MiniBomb") {
    next = new BombCardAction(character, card.GetDamage());
  }
  else if (name == "CrakShot") {
    next = new CrackShotCardAction(character, card.GetDamage());
  }
  else if (name == "Sword") {
    next = new SwordCardAction(character, card.GetDamage());
  }
  else if (name == "ElcPuls1") {
    next = new ElecPulseCardAction(character, card.GetDamage());
  }
  else if (name == "LongSwrd") {
    next = new LongSwordCardAction(character, card.GetDamage());
  }
  else if (name == "WideSwrd") {
    next = new WideSwordCardAction(character, card.GetDamage());
  }
  else if (name == "FireSwrd") {
    auto action = new LongSwordCardAction(character, card.GetDamage());
    action->SetElement(Element::fire);
    next = action;
  }
  else if (name == "AirShot") {
    next = new AirShotCardAction(character, card.GetDamage());
  }
  else if (name == "Thunder") {
    next = new ThunderCardAction(character, card.GetDamage());
  }
  else if (name.substr(0, 4) == "Roll") {
    next = new RollCardAction(character, card.GetDamage());
  }
  else if (name == "ProtoMan") {
    next = new ProtoManCardAction(character, card.GetDamage());
  }
  else if (name == "RockCube") {
    next = new CubeCardAction(character);
  }
  else if (name == "AreaGrab") {
    next = new AreaGrabCardAction(character, 10);
  }
  else if (name == "AntiDmg") {
    next = new AntiDmgCardAction(character, card.GetDamage());
  }
  else if (name == "Barrier") {
    next = new AuraCardAction(character, Aura::Type::BARRIER_10);
  }
  else if (name.substr(0, 7) == "MachGun") {
    next = new MachGunCardAction(character, card.GetDamage());
  }
  else if (name == "HubBatch") {
    next = new HubBatchCardAction(character);
  }
  else if (name == "AirHocky") {
    next = new AirHockeyCardAction(character, card.GetDamage());
  }
  else if (name == "WindRack") {
    next = new WindRackCardAction(character, card.GetDamage());
  }

  return next;
}