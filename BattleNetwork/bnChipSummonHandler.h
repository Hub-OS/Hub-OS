#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnRollHeal.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnTile.h"
#include "Swoosh\Timer.h"

class ChipSummonHandler : public ChipUseListener {
private:
  Player * player;
  double timeInSecs;
  sf::Time duration;
  std::string summon;
  Chip copy;
  std::vector<Entity*> summonedItems; // We must handle our own summoned entites

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

  void SummonEntity(Entity* _new) {
    summonedItems.push_back(_new);
  }

  void RemoveEntity(Entity* _entity) {
    for (auto items = summonedItems.begin(); items != summonedItems.end(); items++) {
      if (*items == _entity) {
        if ((*items)->GetTile()) {
          (*items)->GetTile()->RemoveEntity(*items);
        }

        delete *items;
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

    for (auto items : summonedItems) {
      items->Update((float)_elapsed);
    }
  }

  void OnEnter() { 
    if (summon == "Roll") {
      player->SetAlpha(0);

      Entity* roll = new RollHeal(this, copy.GetDamage());
      SummonEntity(roll);
    }
  }

  void OnLeave() { 
    player->SetAlpha(255);  

    for (auto items : summonedItems) {
      delete items;
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
    }
  }
};
