#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnRollHeal.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCube.h"
#include "bnAura.h"

#include <Swoosh\Timer.h>

class ChipSummonHandler : public ChipUseListener {
private:
  Player * player;
  double timeInSecs;
  sf::Time duration;
  std::string summon;
  Chip copy;

  struct SummonBucket {
    Entity* entity;
    bool persist;

    SummonBucket(Entity* e, bool p) { entity = e; persist = p; }
  };

  std::vector<SummonBucket> summonedItems; // We must handle our own summoned entites

public:
  ChipSummonHandler(Player* _player) : ChipUseListener() { player = _player; duration = sf::seconds(3); timeInSecs = 0; summon = std::string();  }
  ChipSummonHandler(Player& _player) : ChipUseListener() { player = &_player; duration = sf::seconds(3); timeInSecs = 0; summon = std::string(); }

  const bool IsSummonOver() {
    return timeInSecs >= duration.asSeconds() || summon.empty();
  }

  const bool IsSummonsActive() {
    return !IsSummonOver();
  }

  const std::string GetSummonLabel() { return copy.GetShortName(); }

  Player* GetPlayer() {
    return player;
  }

  void SummonEntity(Entity* _new, bool persist=false)  {
    summonedItems.push_back(SummonBucket(_new, persist));
  }

  void RemoveEntity(Entity* _entity) {
    for (auto items = summonedItems.begin(); items != summonedItems.end(); items++) {
      if (items->entity == _entity) {
        if (items->entity->GetTile()) {
          items->entity->GetTile()->RemoveEntity(items->entity);
        }

        delete items->entity;
        summonedItems.erase(items);
        return;
      }
    }
  }

  void Update(double _elapsed) {
    if (summon.empty())
      return;

    player->Update(0);
    player->GetAnimationComponent().Update((float)_elapsed);

    timeInSecs += _elapsed;


    for (auto iter = summonedItems.begin(); iter != summonedItems.end();) {
      if (iter->entity->IsDeleted()) {
        iter->entity->GetField()->RemoveEntity(iter->entity);
        iter = summonedItems.erase(iter);
        continue;
      }
      
      iter++;
    }

    for (auto items : summonedItems) {
        items.entity->Update((float)_elapsed);
    }
  }

  void OnEnter() { 
    if (summon == "Roll") {
      player->SetAlpha(0);

      Entity* roll = new RollHeal(this, copy.GetDamage());
      SummonEntity(roll);
    }
    else if (summon == "RockCube") {
      Entity* cube = new Cube(this->GetPlayer()->GetField(), Team::UNKNOWN);

      Battle::Tile* tile = this->GetPlayer()->GetTile();
      tile = this->GetPlayer()->GetField()->GetAt(tile->GetX()+1, tile->GetY());

      if (tile) {
        this->GetPlayer()->GetField()->AddEntity(cube, tile->GetX(), tile->GetY());

        AUDIO.Play(AudioType::APPEAR);

        // PERSIST. DO NOT ADD TO SUMMONS CLEANUP LIST!
        SummonEntity(cube, true);
      }
    }
    else if (summon == "Barrier") {
      Entity* aura = new Aura(Aura::Type::_100, this->GetPlayer());

      AUDIO.Play(AudioType::APPEAR);

      Battle::Tile* tile = this->GetPlayer()->GetTile();

      if (tile) {
        this->GetPlayer()->GetField()->AddEntity(aura, tile->GetX(), tile->GetY());
      }

      // PERSIST. DO NOT ADD TO SUMMONS CLEANUP LIST!
      SummonEntity(aura, true);
    }
  }

  void OnLeave() { 
    player->SetAlpha(255);  

    for (auto items : summonedItems) {
      if (items.persist) {
        Battle::Tile* tile = items.entity->GetTile();
  
        player->GetField()->RemoveEntity(items.entity);
        player->GetField()->OwnEntity(items.entity, tile->GetX(), tile->GetY());
      }
      else {
        player->GetField()->RemoveEntity(items.entity);
      }
    }

    summonedItems.clear();

    summon = std::string();
  }

  void OnChipUse(Chip& chip, Character& character) {
    player->SetCharging(false);

    std::string name = chip.GetShortName();
    copy = chip;

    if (name.substr(0,4) == "Roll") {
      summon = "Roll";
      timeInSecs = 0;
      duration = sf::seconds(4);
    } else if (name.substr(0, 8) == "RockCube") {
      summon = "RockCube";
      timeInSecs = 0;
      duration = sf::seconds(1);
    }
    else if (name.substr(0, 8) == "Barrier") {
      summon = "Barrier";
      timeInSecs = 0;
      duration = sf::seconds(1);
    }
  }
};
