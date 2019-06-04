/*! \brief Describes what happens when a chip is used by the player navi 
 * 
 * \warning This is based on old system design and will be removed completely
 * 
 *       Reasons for removal:
 *       No reason why this needs to be separate from the enemy chip listener
 *       The code is nearly identical and should be refactored to support being
 *       used by both players and enemies.
 * 
 *       Also chips will be scripted and this logic will be handled by the 
 *       script interpreter (Lua) and this will no longer be used
 */

#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnPlayer.h"
#include "bnFishy.h"
#include "bnTile.h"
#include "bnAirShot.h"
#include "bnCannon.h"
#include "bnBasicSword.h"
#include "bnThunder.h"
#include "bnInvis.h"
#include "bnElecpulse.h"
#include "bnReflectShield.h"
#include "bnHideUntil.h"

class PlayerChipUseListener : public ChipUseListener {
private:
  Player * player; /*!< Entity to listen for */

public:
  PlayerChipUseListener(Player* _player) : ChipUseListener() { player = _player; }
  PlayerChipUseListener(Player& _player) : ChipUseListener() { player = &_player;  }

  /**
   * @brief What happens when a chip is used
   * @param chip Chip used
   * @param character Character using chip
   */
  void OnChipUse(Chip& chip, Character& character) {
    // Player charging is cancelled
    player->SetCharging(false);

    // Identify the chip by the name
    std::string name = chip.GetShortName();

    if (name.substr(0, 5) == "Recov") {
      // Increase player health
      player->SetHealth(player->GetHealth() + chip.GetDamage());
      
      // Play sound
      AUDIO.Play(AudioType::RECOVER);

      // Change the animation back to IDLE when finished
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      // Play heal animation with animation end callback
      player->SetAnimation(PLAYER_HEAL, onFinish);
    }
    else if (name == "CrckPanel") {
      // Crack the top, middle, and bottom row in front of player
      Battle::Tile* top = player->GetField()->GetAt(player->GetTile()->GetX() + 1, 1);
      Battle::Tile* mid = player->GetField()->GetAt(player->GetTile()->GetX() + 1, 2);
      Battle::Tile* low = player->GetField()->GetAt(player->GetTile()->GetX() + 1, 3);

      // If the tiles are valid, set their state to CRACKED
      if (top) { top->SetState(TileState::CRACKED); }
      if (mid) { mid->SetState(TileState::CRACKED); }
      if (low) { low->SetState(TileState::CRACKED); }

      AUDIO.Play(AudioType::PANEL_CRACK);
    }
    else if (name == "Invis") {
      // Create an invisible component. This handles the logic for timed invis
      Component* invis = new Invis(player);
      
      // Add the component to player
      player->RegisterComponent(invis);
    }
    else if (name == "Rflector") {
      // Create a new reflect shield component. This handles the logic for shields.
      ReflectShield* reflect = new ReflectShield(player);
      
      // Add the component to the player
      player->RegisterComponent(reflect);

      // Play the appear sound
      AUDIO.Play(AudioType::APPEAR);

      // Add shield artifact on the same layer as player
      Battle::Tile* tile = player->GetTile();

      if (tile) {
        this->player->GetField()->AddEntity(*reflect, tile->GetX(), tile->GetY());
      }
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
      fishy->SetDirection(Direction::RIGHT);
      
      // Condition to end hide
      HideUntil::Callback until = [fishy]() { return fishy->IsDeleted(); };
      
      // First argument is the entity to hide
      // Second argument is the query functor
      HideUntil* fishyStatus = new HideUntil(player, until);
      player->RegisterComponent(fishyStatus);

      Battle::Tile* tile = player->GetTile();

      if (tile) {
        this->player->GetField()->AddEntity(*fishy, tile->GetX(), tile->GetY());
      }
    }
    else if (name == "XtrmeCnnon") {
      // Spawn a cannon spell on all rows
      Cannon* xtreme1 = new Cannon(player->GetField(), player->GetTeam(), chip.GetDamage());
      Cannon* xtreme2 = new Cannon(player->GetField(), player->GetTeam(), chip.GetDamage());
      Cannon* xtreme3 = new Cannon(player->GetField(), player->GetTeam(), chip.GetDamage());

      // Shake the camera for 1 second with an intensity of 25 pixels
      ENGINE.GetCamera()->ShakeCamera(25, sf::seconds(1));

      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };
      player->SetAnimation(PLAYER_CANNON, onFinish);
      AUDIO.Play(AudioType::CANNON);

      xtreme1->SetDirection(Direction::RIGHT);
      xtreme2->SetDirection(Direction::RIGHT);
      xtreme3->SetDirection(Direction::RIGHT);

      player->GetField()->AddEntity(*xtreme1, player->GetTile()->GetX(), 1);
      player->GetField()->AddEntity(*xtreme2, player->GetTile()->GetX(), 2);
      player->GetField()->AddEntity(*xtreme3, player->GetTile()->GetX(), 3);
    }
    else if (name == "Cannon") {
      // Spawn a single cannon instance on the tile in front of the player
      Cannon* cannon = new Cannon(player->GetField(), player->GetTeam(), chip.GetDamage());
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };
      player->SetAnimation(PLAYER_CANNON, onFinish);
      AUDIO.Play(AudioType::CANNON);

      cannon->SetDirection(Direction::RIGHT);

      player->GetField()->AddEntity(*cannon, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
    }
    else if (name == "Swrd") {
      // Play sword animation and add sword spell in the tile in front
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      player->SetAnimation(PLAYER_SWORD, onFinish);

      BasicSword* sword = new BasicSword(player->GetField(), player->GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::SWORD_SWING);

      player->GetField()->AddEntity(*sword, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
    }
    else if (name == "Elecplse") {
      // Spawn an elecpulse attack
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      player->SetAnimation(PLAYER_SHOOTING, onFinish);

      Elecpulse* pulse = new Elecpulse(player->GetField(), player->GetTeam(), chip.GetDamage());
    
      AUDIO.Play(AudioType::ELECPULSE);

      player->GetField()->AddEntity(*pulse, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
    }
    else if (name == "LongSwrd") {
        
      // Play sword animation
      // Spawn two sword spells on the tile in front and one after that
      // This effecitvely works as long sword
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      player->SetAnimation(PLAYER_SWORD, onFinish);

      BasicSword* sword = new BasicSword(player->GetField(), player->GetTeam(), chip.GetDamage());
      BasicSword* sword2 = new BasicSword(player->GetField(), player->GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::SWORD_SWING);

      // We cant spawn an entity on a tile that doesn't exist
      if (player->GetField()->GetAt(player->GetTile()->GetX() + 1, player->GetTile()->GetY())) {
        player->GetField()->AddEntity(*sword, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
      }

      if (player->GetField()->GetAt(player->GetTile()->GetX() + 2, player->GetTile()->GetY())) {
        player->GetField()->AddEntity(*sword2, player->GetTile()->GetX() + 2, player->GetTile()->GetY());
      }
    }
    else if (name == "WideSwrd") {
      // Same as long sword
      // Spawn two sword spells in front of the player and one above that
      // This effecitvely works as a wide sword
      
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      player->SetAnimation(PLAYER_SWORD, onFinish);

      BasicSword* sword = new BasicSword(player->GetField(), player->GetTeam(), chip.GetDamage());
      BasicSword* sword2 = new BasicSword(player->GetField(), player->GetTeam(), chip.GetDamage());

      AUDIO.Play(AudioType::SWORD_SWING);

      if (player->GetField()->GetAt(player->GetTile()->GetX() + 1, player->GetTile()->GetY())) {
        player->GetField()->AddEntity(*sword, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
      }

      if (player->GetField()->GetAt(player->GetTile()->GetX() + 1, player->GetTile()->GetY() + 1)) {
        player->GetField()->AddEntity(*sword2, player->GetTile()->GetX() + 1, player->GetTile()->GetY() + 1);
      }
    }
    else if (name == "AirShot1") {
      // Spawn an airshot spell
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      player->SetAnimation(PLAYER_SHOOTING, onFinish);

      AUDIO.Play(AudioType::SPREADER);

      AirShot* airshot = new AirShot(player->GetField(), player->GetTeam(), chip.GetDamage());
      airshot->SetDirection(Direction::RIGHT);

      player->GetField()->AddEntity(*airshot, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
    }
    else if (name == "Thunder") {

      // Spawn the thunder spell to track enemies
      auto onFinish = [this]() { this->player->SetAnimation(PLAYER_IDLE);  };

      player->SetAnimation(PLAYER_SHOOTING, onFinish);

      Thunder* thunder = new Thunder(player->GetField(), player->GetTeam());
      player->GetField()->AddEntity(*thunder, player->GetTile()->GetX() + 1, player->GetTile()->GetY());
    }
  }
};
