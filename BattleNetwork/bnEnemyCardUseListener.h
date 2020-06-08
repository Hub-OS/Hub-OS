#pragma once

#include "bnResourceHandle.h"
#include "bnCardUseListener.h"
#include "bnCharacter.h"
#include "bnTile.h"
#include "bnCannon.h"
#include "bnBasicSword.h"

/**
 * @class EnemyCardUseListener
 * @author mav
 * @date 04/05/19
 * @brief Describes what should happen when an enemy uses a card
 * @see PlayerCardUseListner.h
 * @warning Based on legacy design. Ultimately there should be one card use listener
 * 
 * No comments because it is a carbon copy of PlayerCardUseListener
 * and needs to be removed from the engine.
 */
class EnemyCardUseListener : public CardUseListener, public ResourceHandle {
private:
 
public:
  EnemyCardUseListener() = default;

  virtual void OnCardUse(Battle::Card& card, Character& user) {
    std::string name = card.GetShortName();

    if (name.substr(0, 5) == "Recov") {
      user.SetHealth(user.GetHealth() + card.GetDamage());
      Audio()().Play(AudioType::RECOVER);
    }
    else if (name == "CrckPanel") {
      Battle::Tile* top = user.GetField()->GetAt(user.GetTile()->GetX() - 1, 1);
      Battle::Tile* mid = user.GetField()->GetAt(user.GetTile()->GetX() - 1, 2);
      Battle::Tile* low = user.GetField()->GetAt(user.GetTile()->GetX() - 1, 3);

      if (top) { top->SetState(TileState::cracked); }
      if (mid) { mid->SetState(TileState::cracked); }
      if (low) { low->SetState(TileState::cracked); }

      Audio()().Play(AudioType::PANEL_CRACK);
    }
    else if (name == "Invis") {
      Audio()().Play(AudioType::INVISIBLE);
      //user.SetCloakTimer(20); // TODO: make this a time-based component
    }
    else if (name == "Cannon") {
      Cannon* cannon = new Cannon(user.GetField(), user.GetTeam(), card.GetDamage());

      Audio()().Play(AudioType::CANNON);

      cannon->SetDirection(Direction::left);

      user.GetField()->AddEntity(*cannon, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
    }
    else if (name == "Swrd") {
      BasicSword* sword = new BasicSword(user.GetField(), user.GetTeam(), card.GetDamage());

      Audio()().Play(AudioType::SWORD_SWING);

      user.GetField()->AddEntity(*sword, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
    }
    else if (name == "LongSwrd") {
      BasicSword* sword = new BasicSword(user.GetField(), user.GetTeam(), card.GetDamage());
      BasicSword* sword2 = new BasicSword(user.GetField(), user.GetTeam(), card.GetDamage());

      Audio()().Play(AudioType::SWORD_SWING);

      if (user.GetField()->GetAt(user.GetTile()->GetX() - 1, user.GetTile()->GetY())) {
        user.GetField()->AddEntity(*sword, user.GetTile()->GetX() - 1, user.GetTile()->GetY());
      }

      if (user.GetField()->GetAt(user.GetTile()->GetX() - 2, user.GetTile()->GetY())) {
        user.GetField()->AddEntity(*sword2, user.GetTile()->GetX() - 2, user.GetTile()->GetY());
      }
    }
    else if (name == "WideSwrd") {
      BasicSword* sword = new BasicSword(user.GetField(), user.GetTeam(), card.GetDamage());
      BasicSword* sword2 = new BasicSword(user.GetField(), user.GetTeam(), card.GetDamage());

      Audio()().Play(AudioType::SWORD_SWING);

      if (user.GetField()->GetAt(user.GetTile()->GetX() + 1, user.GetTile()->GetY())) {
        user.GetField()->AddEntity(*sword, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
      }

      if (user.GetField()->GetAt(user.GetTile()->GetX() + 1, user.GetTile()->GetY() + 1)) {
        user.GetField()->AddEntity(*sword2, user.GetTile()->GetX() + 1, user.GetTile()->GetY() + 1);
      }
    }
  }
};