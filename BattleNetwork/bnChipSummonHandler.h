#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnRollHeal.h"
#include "bnNinjaAntiDamage.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCube.h"
#include "bnAura.h"
#include "bnPanelGrab.h"

#include <Swoosh/Timer.h>

/*
TODO: use action lists where possible
      Allow any entity to be used by the chip summon handler
      Refactor this horrible design
*/
class ChipSummonHandler : public ChipUseListener {
private:
  Player * player;
  Character * other;
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
  ChipSummonHandler(Player* _player) : ChipUseListener() { other = nullptr; player = _player; duration = sf::seconds(3); timeInSecs = 0; summon = std::string();  }
  ChipSummonHandler(Player& _player) : ChipUseListener() { other = nullptr; player = &_player; duration = sf::seconds(3); timeInSecs = 0; summon = std::string(); }

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
          items->entity->GetTile()->RemoveEntityByID(items->entity->GetID());
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

    if (!other) {
      player->Update(0);
      player->GetAnimationComponent().Update((float)_elapsed);
    }
    else {
      other->Update(0);
      //other->GetAnimationComponent().Update((float)_elapsed);
    }

    timeInSecs += _elapsed;


    for (auto iter = summonedItems.begin(); iter != summonedItems.end();) {
      if (iter->entity->IsDeleted()) {
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
    Character* summonedBy = player;

    if (other) {
      summonedBy = other;
    }

    if (summon == "Roll") {
      summonedBy->Hide();

      Entity* roll = new RollHeal(this, copy.GetDamage());
      SummonEntity(roll);
    }
    else if (summon == "RockCube") {
      Obstacle* cube = new Cube(summonedBy->GetField(), Team::UNKNOWN);

      Battle::Tile* tile = summonedBy->GetTile();

      if (summonedBy->GetTeam() == Team::RED) {
        tile = summonedBy->GetField()->GetAt(tile->GetX() + 1, tile->GetY());
      }
      else {
        tile = summonedBy->GetField()->GetAt(tile->GetX() - 1, tile->GetY());
      }

      if (tile) {
        summonedBy->GetField()->AddEntity(*cube, tile->GetX(), tile->GetY());

        AUDIO.Play(AudioType::APPEAR);

        // PERSIST. DO NOT ADD TO SUMMONS CLEANUP LIST!
        SummonEntity(cube, true);
      }
    }
    else if (summon == "AreaGrab") {
      PanelGrab** grab = new PanelGrab*[3];
      
      for (int i = 0; i < 3; i++) {
        grab[i] = new PanelGrab(summonedBy->GetField(), summonedBy->GetTeam(), 0.5);
      }

      Battle::Tile** tile = new Battle::Tile*[3];

      Field* f = summonedBy->GetField();
      
      // Read team grab scans from left to right
      if (summonedBy->GetTeam() == Team::RED) {
        for (int i = 0; i < f->GetHeight(); i++) {
          int index = 1;
          while (f->GetAt(index, i+1) && f->GetAt(index, i+1)->GetTeam() == Team::RED) {
            index++;
          }

          tile[i] = f->GetAt(index, i+1);

          if (tile[i]) {
            summonedBy->GetField()->AddEntity(*grab[i], tile[i]->GetX(), tile[i]->GetY());

            // PERSIST. DO NOT ADD TO SUMMONS CLEANUP LIST!
            SummonEntity(grab[i], true);
          }
          else {
            delete grab[i];
          }
        }
      } else if (summonedBy->GetTeam() == Team::BLUE) {
        // Blue team grab scans from right to left

        for (int i = 0; i < f->GetHeight(); i++) {
          int index = f->GetWidth();
          while (f->GetAt(index, i+1) && f->GetAt(index, i+1)->GetTeam() == Team::BLUE) {
            index--;
          }

          tile[i] = f->GetAt(index, i+1);

          if (tile[i]) {
            summonedBy->GetField()->AddEntity(*grab[i], tile[i]->GetX(), tile[i]->GetY());

            // PERSIST. DO NOT ADD TO SUMMONS CLEANUP LIST!
            SummonEntity(grab[i], true);
          }
          else {
            delete grab[i];
          }
        }
      }

      // cleanup buckets
      delete[] tile;
      delete[] grab;
    }

    else if (summon == "Antidamg") {
      NinjaAntiDamage* antidamage = new NinjaAntiDamage(summonedBy);
      summonedBy->RegisterComponent(antidamage);

      AUDIO.Play(AudioType::APPEAR);
    }
    else if (summon == "Barrier") {
      Aura* aura = new Aura(Aura::Type::BARRIER_100, summonedBy);
      //this->GetPlayer()->RegisterComponent(aura);

      AUDIO.Play(AudioType::APPEAR);

      Battle::Tile* tile = summonedBy->GetTile();

      if (tile) {
        summonedBy->GetField()->AddEntity(*aura, tile->GetX(), tile->GetY());
      }

      // PERSIST. DO NOT ADD TO SUMMONS CLEANUP LIST!
      SummonEntity(aura, true);
    }

    other = nullptr;
  }

  void OnLeave() { 
    player->Reveal();

    if (other) {
      other->Reveal();
    }

    for (auto items : summonedItems) {
      if (!items.persist) {
        items.entity->Delete();
      }
    }

    summonedItems.clear();

    summon = std::string();
  }

  void OnChipUse(Chip& chip, Character& character) {
    if (dynamic_cast<Character*>(player) == &character) {
      player->SetCharging(false);
      other = nullptr;
    }
    else {
      other = &character;
    }

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
    else if (name.substr(0, 7) == "Barrier") {
      summon = "Barrier";
      timeInSecs = 0;
      duration = sf::seconds(1);
    }
    else if (name == "Antidamg") {
      summon = "Antidamg";
      timeInSecs = 0;
      duration = sf::seconds(1);
    }
    else if (name == "AreaGrab") {
      summon = name;
      timeInSecs = 0;
      duration = sf::seconds(1);
    }
  }
};
