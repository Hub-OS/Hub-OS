#pragma once

#include "bnAudioResourceManager.h"
#include "bnCardUseListener.h"
#include "bnRollHeal.h"
#include "bnProtoManSummon.h"
#include "bnNinjaAntiDamage.h"
#include "bnPlayer.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCube.h"
#include "bnAura.h"
#include "bnCallback.h"
#include "bnPanelGrab.h"

#include <Swoosh/Timer.h>
#include <deque>

/**
 * @class CardSummonHandler
 * @author mav
 * @date 05/05/19
 * @brief Summon system freezes time and plays out before resuming battle
 * @warning This code was written as a proof of concept and needs to be redesigned entirely
 * 
 * TODO: use action lists where possible
 * Allow any entity to be used by the card summon handler
 * Refactor this horrible design
 * 
 * I'm not going to spend a lot of time explaining how this works because it's crap.
 * Essentially matching cards in the listener OnUse() queues the caller, card, and freeze time information
 * 
 * when the handler is updated, the loop dequeues the info and calls OnEnter()
 * This may summon entities and puts them in their own update bucket separate from the field's.
 * After the update ends it calls OnLeave() and it'll flag whether or not it has more in the queue.
 * 
 * This repeats until the queue is empty. Then the IsSummonOver() will return true.
 * 
 */
class CardSummonHandler : public CardUseListener {
private:
  std::list<Callback<void()>> tfcOnCompleteCallbacks;

  struct CardSummonQueue {
    std::deque<Character*> callers;
    std::deque<Battle::Card> cards;
    std::deque<sf::Time> durations;
    std::deque<uint64_t> timestamps;

    const size_t Size() const {
      return callers.size();
    }

    void Add(Battle::Card card, Character& caller, sf::Time duration, long long timestamp) {
      callers.push_back(&caller);
      cards.push_back(card);
      durations.push_back(duration);
      timestamps.push_back(timestamp);
    }

    void AddFront(Battle::Card card, Character& caller, sf::Time duration, long long timestamp) {
      callers.push_front(&caller);
      cards.push_front(card);
      durations.push_front(duration);
      timestamps.push_front(timestamp);
    }

    void Pop() {
      if (Size() > 0) {
        callers.pop_front();
        cards.pop_front();
        durations.pop_front();
        timestamps.pop_front();
      }
    }

    const long long GetTimestamp() const {
      if (timestamps.size()) {
        return timestamps.front();
      }
      else {
        return 0;
      }
    }

    Character* GetCaller() {
      return callers.front();
    }

    Battle::Card GetCard() {
      return cards.front();
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

  Character *other;
  double timeInSecs;
  sf::Time duration;
  std::string summon;
  Battle::Card copy;
  Team callerTeam;

  CardSummonQueue queue;
  std::vector<SummonBucket> summonedItems; // We must handle our own summoned entites

public:
  CardSummonHandler() : CardUseListener(), copy() { other = nullptr; duration = sf::seconds(0); timeInSecs = 0; summon = std::string(); }

  const bool IsSummonOver() const {
    return ((timeInSecs >= queue.GetDuration()));
  }

  const bool HasMoreInQueue() const {
    return queue.Size() > 0;
  }

  const bool IsSummonActive() const {
    return !IsSummonOver() && !summon.empty();
  }

  const std::string GetSummonLabel() { return queue.GetCard().GetShortName(); }

  const Team GetCallerTeam() const {
    if (HasMoreInQueue()) {
      return queue.callers.front()->GetTeam();
    }

    return Team::unknown;
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
    //std::cout << "in summons update" << std::endl;

    timeInSecs += _elapsed;
  }

  void OnEnter() { 
    /*

    Character* summonedBy = queue.GetCaller();

    std::cout << "[" << summonedBy->GetName() << ", " << queue.GetCard().GetShortName() << ", " << queue.GetDuration() << "sec]" << std::endl;

    std::string name = queue.GetCard().GetShortName();

    if (name.substr(0, 4) == "Roll") {
      summon = "Roll";

    }
    else if (name.substr(0, 8) == "RockCube") {
      summon = "RockCube";

    }
    else if (name.substr(0, 7) == "Barrier") {
      summon = "Barrier";

    }
    else if (name == "AntiDmg") {
      summon = "AntiDmg";

    }
    else if (name == "AreaGrab") {
      summon = name;

    } else if(name == "ProtoMan") {
    summon = name; 
  }

    // TODO: Don't use summon, use queue.GetCard();
    if (summon == "Roll") {
      summonedBy->Hide();

      Entity* roll = new RollHeal(this, queue.GetCard().GetDamage());
      SummonEntity(roll);
    } else if(summon == "ProtoMan") {
      summonedBy->Hide();
      Entity* proto = new ProtoManSummon(this);
      SummonEntity(proto);
    }
    else if (summon == "RockCube") {
      Cube* cube = new Cube(summonedBy->GetField(), Team::unknown);

      Battle::Tile* tile = summonedBy->GetTile();

      if (summonedBy->GetTeam() == Team::red) {
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

        Callback<void()> callback;
        callback.Slot([cube]() {
          if (!cube->DidSpawnCorrectly()) {
            cube->Delete();
          }
        });

        tfcOnCompleteCallbacks.insert(tfcOnCompleteCallbacks.begin(), callback);
      }
    }
    else if (summon == "AreaGrab") {
      PanelGrab** grab = new PanelGrab*[3];
      
      for (int i = 0; i < 3; i++) {
        grab[i] = new PanelGrab(summonedBy->GetField(), summonedBy->GetTeam(), 0.25f);
      }

      Battle::Tile** tile = new Battle::Tile*[3];

      Field* f = summonedBy->GetField();
      // Read team grab scans from left to right
      if (summonedBy->GetTeam() == Team::red) {
        int minIndex = 6;

        for (int i = 0; i < f->GetHeight(); i++) {
          int index = 1;
          while (f->GetAt(index, i + 1) && f->GetAt(index, i + 1)->GetTeam() == Team::red) {
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
      } else if (summonedBy->GetTeam() == Team::blue) {
        // Blue team grab scans from right to left

        int maxIndex = 1;

        for (int i = 0; i < f->GetHeight(); i++) {
          int index = f->GetWidth();
          while (f->GetAt(index, i + 1) && f->GetAt(index, i + 1)->GetTeam() == Team::blue) {
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
    else if (summon == "AntiDmg") {
      NinjaAntiDamage* antidamage = new NinjaAntiDamage(summonedBy);
      summonedBy->RegisterComponent(antidamage);

      AUDIO.Play(AudioType::APPEAR);
    }
    else if (summon == "Barrier") {
      Aura* aura = new Aura(Aura::Type::BARRIER_10, summonedBy);
      AUDIO.Play(AudioType::APPEAR);
    }
    */
  }

  void OnLeave() { 
    queue.Pop();

    std::cout << "Summon onLeave [queue size is " << queue.Size() << "]" << std::endl;

    /*for (auto items : summonedItems) {
      if (!items.persist) {
        items.entity->Delete();
      }
    }*/

    summonedItems.clear();

    summon = std::string();

    if (HasMoreInQueue()) {
      timeInSecs = 0;
    }
    else {
      for (auto&& fn : tfcOnCompleteCallbacks) {
        fn();
      }

      tfcOnCompleteCallbacks.clear();
    }
  }

  // when timestamp is 0, we do not check the queue in respect to time
  void OnCardUse(Battle::Card& card, Character& character, long long timestamp) {
    auto timeDiff = CurrentTime::AsMilli() - timestamp;
   
    if (timeDiff >= (3 * 1000)) {
      Logger::Logf("Not spawning chip because timeDiff was %i", timeDiff);

      // 5 seconds or more delay is too long ago. Ignore...
      return;
    }

    std::cout << "on cards use " << card.GetShortName() << " by " << character.GetName() << std::endl;
    std::string name = card.GetShortName();

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
    else if (name == "AntiDmg") {
      summon = "AntiDmg";
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
      timeInSecs = 0;
      copy = Battle::Card{};
      add = false;
    }

    summon.clear();

    if (add) {
      if (timestamp && queue.GetTimestamp() > timestamp) {
        queue.AddFront(card, character, duration, timestamp);
      }
      else {
        queue.Add(card, character, duration, 0);
      }
    }
  }
};
