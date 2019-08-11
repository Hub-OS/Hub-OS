#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnRollHeal.h"
#include "bnProtoManSummon.h"
#include "bnNinjaAntiDamage.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCube.h"
#include "bnAura.h"
#include "bnPanelGrab.h"

#include <Swoosh/Timer.h>
#include <queue>

/**
 * @class ChipSummonHandler
 * @author mav
 * @date 05/05/19
 * @brief Summon system freezes time and plays out before resuming battle
 * @warning This code was written as a proof of concept and needs to be redesigned entirely
 * 
 * TODO: use action lists where possible
 * Allow any entity to be used by the chip summon handler
 * Refactor this horrible design
 * 
 * I'm not going to spend a lot of time explaining how this works because it's crap.
 * Essentially matching chips in the listener OnUse() queues the caller, chip, and freeze time information
 * 
 * when the handler is updated, the loop dequeues the info and calls OnEnter()
 * This may summon entities and puts them in their own update bucket separate from the field's.
 * After the update ends it calls OnLeave() and it'll flag whether or not it has more in the queue.
 * 
 * This repeats until the queue is empty. Then the IsSummonOver() will return true.
 * 
 */
class ChipSummonHandler : public ChipUseListener {
private:
  struct ChipSummonQueue {
    std::queue<Character*> callers;
    std::queue<Chip> chips;
    std::queue<sf::Time> durations;

    const size_t Size() const {
      return callers.size();
    }

    void Add(Chip chip, Character& caller, sf::Time duration) {
      callers.push(&caller);
      chips.push(chip);
      durations.push(duration);
    }

    void Pop() {
      if (Size() > 0) {
        callers.pop();
        chips.pop();
        durations.pop();
      }
    }

    Character* GetCaller() {
      return callers.front();
    }

    Chip GetChip() {
      return chips.front();
    }

    double GetDuration() const {
      if (Size() > 0) {
        return durations.front().asSeconds();
      }

      return 0;
    }
  };

  struct SummonBucket {
    Entity* entity;
    bool persist;

    SummonBucket(Entity* e, bool p) { entity = e; persist = p; }
  };

  Player * player;
  Character * other;
  double timeInSecs;
  sf::Time duration;
  std::string summon;
  Chip copy;
  Team callerTeam;

  ChipSummonQueue queue;
  std::vector<SummonBucket> summonedItems; // We must handle our own summoned entites

public:
  ChipSummonHandler(Player* _player) : ChipUseListener() { other = nullptr; player = _player; duration = sf::seconds(0); timeInSecs = 0; summon = std::string();  }
  ChipSummonHandler(Player& _player) : ChipUseListener() { other = nullptr; player = &_player; duration = sf::seconds(0); timeInSecs = 0; summon = std::string(); }

  const bool IsSummonOver() const {
    return ((timeInSecs >= queue.GetDuration()));
  }

  const bool HasMoreInQueue() const {
    return queue.Size() > 0;
  }

  const bool IsSummonActive() const {
    return !IsSummonOver() && !summon.empty();
  }

  const std::string GetSummonLabel() { return queue.GetChip().GetShortName(); }

  const Team GetCallerTeam() const {
    if (HasMoreInQueue()) {
      return queue.callers.front()->GetTeam();
    }

    return Team::UNKNOWN;
  }

  Character* GetCaller() {
    return queue.GetCaller();
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
    std::cout << "in summons update" << std::endl;

    // queue.GetCaller()->Update(0);

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
    std::cout << "Summons onEnter ";

    Character* summonedBy = queue.GetCaller();

    std::cout << "[" << summonedBy->GetName() << ", " << queue.GetChip().GetShortName() << ", " << queue.GetDuration() << "sec]" << std::endl;

    std::string name = queue.GetChip().GetShortName();

    if (name.substr(0, 4) == "Roll") {
      summon = "Roll";

    }
    else if (name.substr(0, 8) == "RockCube") {
      summon = "RockCube";

    }
    else if (name.substr(0, 7) == "Barrier") {
      summon = "Barrier";

    }
    else if (name == "Antidamg") {
      summon = "Antidamg";

    }
    else if (name == "AreaGrab") {
      summon = name;

    } else if(name == "ProtoMan") {
		summon = name; 
	}

    // TODO: Don't use summon, use queue.GetChip();
    if (summon == "Roll") {
      summonedBy->Hide();

      Entity* roll = new RollHeal(this, copy.GetDamage());
      SummonEntity(roll);
    } else if(summon == "ProtoMan") {
		summonedBy->Hide();
		Entity* proto = new ProtoManSummon(this);
		SummonEntity(proto);
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
        int minIndex = 6;

        for (int i = 0; i < f->GetHeight(); i++) {
          int index = 1;
          while (f->GetAt(index, i + 1) && f->GetAt(index, i + 1)->GetTeam() == Team::RED) {
            index++;
          }

          minIndex = std::min(minIndex, index);
        }

        for (int i = 0; i < f->GetHeight(); i++) {
          tile[i] = f->GetAt(minIndex, i + 1);

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

        int maxIndex = 1;

        for (int i = 0; i < f->GetHeight(); i++) {
          int index = f->GetWidth();
          while (f->GetAt(index, i + 1) && f->GetAt(index, i + 1)->GetTeam() == Team::BLUE) {
            index--;
          }

          maxIndex = std::max(maxIndex, index);
        }

        for (int i = 0; i < f->GetHeight(); i++) {
          tile[i] = f->GetAt(maxIndex, i+1);

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
      AUDIO.Play(AudioType::APPEAR);
    }
  }

  void OnLeave() { 
    queue.Pop();

    std::cout << "Summon onLeave [queue size is " << queue.Size() << "]" << std::endl;

    for (auto items : summonedItems) {
      if (!items.persist) {
        items.entity->Delete();
      }
    }

    summonedItems.clear();

    summon = std::string();

    if (HasMoreInQueue()) {
      timeInSecs = 0;
    }
  }

  void OnChipUse(Chip& chip, Character& character) {
    std::cout << "on chips use " << chip.GetShortName() << " by " << character.GetName() << std::endl;
    std::string name = chip.GetShortName();

    bool add = true;

    if (name.substr(0, 4) == "Roll") {
      summon = "Roll";
      timeInSecs = 0;
      duration = sf::seconds(4);
    }
    else if(name == "ProtoMan") {
		summon = "ProtoMan";
		timeInSecs = 0;
		duration = sf::seconds(3);
	}
    else if (name.substr(0, 8) == "RockCube") {
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
    else {
      summon.clear();
      timeInSecs = duration.asSeconds() + 1;
      copy = Chip();
      add = false;
    }

    summon.clear();

    if (add) {
      std::cout << "[Summon queued]" << std::endl;
      queue.Add(chip, character, duration);
    }
  }
};
