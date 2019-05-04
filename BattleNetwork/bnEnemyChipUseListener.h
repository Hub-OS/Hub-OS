#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnCharacter.h"
#include "bnTile.h"
#include "bnCannon.h"
#include "bnBasicSword.h"

/**
 * @class EnemyChipUseListener
 * @author mav
 * @date 04/05/19
 * @file bnEnemyChipUseListener.h
 * @brief Describes what should happen when an enemy uses a chip
 * @see PlayerChipUseListner.h
 * @warning Based on legacy design. Ultimately there should be one chip use listener
 * 
 * No comments because it is a carbon copy of PlayerChipUseListener
 * and needs to be removed from the engine.
 */
class EnemyChipUseListener : public ChipUseListener {
private:
 
public:
  EnemyChipUseListener() = default;

  virtual void OnChipUse(Chip& chip, Character& user) {
    std::string name = chip.GetShortName();

    if (name.substr(0, 5) == "Recov") {
      user.SetHealth(user.GetHealth() + chip.GetDamage());
      AUDIO.Play(AudioType::RECOVER);
    }
    else if (name == "CrckPanel") {
      Battle::Tile* top = user.GetField()->GetAt(user.GetTile()->GetX() - 1, 1);
      Battle::Tile* mid = user.GetField()->GetAt(user.GetTile()->GetX() - 1, 2);
      Battle::Tile* low = user.GetField()->GetAt(user.GetTile()->GetX() - 1, 3);

      if (top) { top->SetState(TileState::CRACKED); }
      if (mid) { mid->SetState(TileState::CRACKED); }
      if (low) { low->SetState(TileState::CRACKED); }

      AUDIO.Play(AudioType::PANEL_CRACK);
    }
    else if (name == "Invis") {
      AUDIO.Play(AudioType::INVISIBLE);
      //user.SetCloakTimer(20); // TODO: make this a time-based component
    }
    else if (name == "Cannon") {
      Cannon* cannon = new Cannon(user.GetField(), user.GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::CANNON);

      cannon->SetDirection(Direction::LEFT);

      user.GetField()->AddEntity(*cannon, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
    }
    else if (name == "Swrd") {
      BasicSword* sword = new BasicSword(user.GetField(), user.GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::SWORD_SWING);

      user.GetField()->AddEntity(*sword, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
    }
    else if (name == "LongSwrd") {
      BasicSword* sword = new BasicSword(user.GetField(), user.GetTeam(), chip.GetDamage());
      BasicSword* sword2 = new BasicSword(user.GetField(), user.GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::SWORD_SWING);

      if (user.GetField()->GetAt(user.GetTile()->GetX() - 1, user.GetTile()->GetY())) {
        user.GetField()->AddEntity(*sword, user.GetTile()->GetX() - 1, user.GetTile()->GetY());
      }

      if (user.GetField()->GetAt(user.GetTile()->GetX() - 2, user.GetTile()->GetY())) {
        user.GetField()->AddEntity(*sword2, user.GetTile()->GetX() - 2, user.GetTile()->GetY());
      }
    }
    else if (name == "WideSwrd") {
      BasicSword* sword = new BasicSword(user.GetField(), user.GetTeam(), chip.GetDamage());
      BasicSword* sword2 = new BasicSword(user.GetField(), user.GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::SWORD_SWING);

      if (user.GetField()->GetAt(user.GetTile()->GetX() + 1, user.GetTile()->GetY())) {
        user.GetField()->AddEntity(*sword, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
      }

      if (user.GetField()->GetAt(user.GetTile()->GetX() + 1, user.GetTile()->GetY() + 1)) {
        user.GetField()->AddEntity(*sword2, user.GetTile()->GetX() + 1, user.GetTile()->GetY() + 1);
      }
    }
  }
};