#pragma once
#include "bnCharacter.h"
#include "bnMeta.h"
#include "bnBattleItem.h"
#include <vector>
#include <map>
#include <stdexcept>

class Mob
{
public:
  struct MobData {
    Character* mob;
    int tileX;
    int tileY;
    unsigned index;
  };

private:
  std::vector<MobData*> spawn;
  std::vector<MobData*>::iterator iter;
  std::vector<std::function<void(Character*)>> defaultStateInvokers;
  std::vector<std::function<void(Character*)>> pixelStateInvokers;
  std::multimap<int, BattleItem> rewards;
  bool nextReady;
  Field* field;
  bool isBoss;
public:
  Mob(Field* _field) {
    nextReady = true;
    field = _field;
    isBoss = false;
    iter = spawn.end();
  }

  ~Mob() {
    Cleanup();
  }

  // Cap ranks between 1-10 and where 11 is Rank S
  void RegisterRankedReward(int rank, BattleItem item) {
    rank = std::max(1, rank);
    rank = std::min(11, rank);

    rewards.insert(std::make_pair(rank, item));
  }

  // TODO: Off chance that there's no item
  BattleItem* GetRankedReward(int score) {
    if (rewards.empty()) {
      return nullptr;
    }
    
    // Collect only the items we can be rewarded with...
    std::vector<BattleItem> possible;

    // Populate the possible
    std::multimap<int, BattleItem>::iterator mapIter = rewards.begin();

    while (mapIter != rewards.end()) {
      if (mapIter->first <= score) {
        possible.push_back(mapIter->second);
      }

      mapIter++;
    }

    if (possible.empty()) {
      return nullptr;
    }

    int random = rand() % possible.size();

    std::vector<BattleItem>::iterator possibleIter;
    possibleIter = possible.begin();
      
    while (random > 0) {
      --random;
      possibleIter++;
    }

    return new BattleItem(*possibleIter);
  }

  void Cleanup() {
    for (int i = 0; i < spawn.size(); i++) {
      delete spawn[i]->mob;
      delete spawn[i];
    }
    field = nullptr;
    spawn.clear();
  }

  Field* GetField() {
    return field;
  }

  const int GetMobCount() {
    return (int)spawn.size();
  }

  void ToggleBossFlag() {
    isBoss = !isBoss;
  }

  bool IsBoss() {
    return isBoss;
  }

  const Character& GetMobAt(int index) {
    if (index < 0 || index >= spawn.size()) {
      throw new std::runtime_error(std::string("Invalid index range for Mob::GetMobAt()"));
    }
    return *spawn[index]->mob;
  }

  const bool NextMobReady() {
    return (nextReady && !IsSpawningDone());
  }

  const bool IsSpawningDone() {
    return (iter == spawn.end() && nextReady);
  }

  const bool IsCleared() {
    for (int i = 0; i < spawn.size(); i++) {
      if (!spawn[i]->mob->IsDeleted()) {
        return false;
      }
    }

    return true;
  }

  void FlagNextReady() {
    this->nextReady = true;
  }

  void DefaultState() {
    for (int i = 0; i < defaultStateInvokers.size(); i++) {
      defaultStateInvokers[i](spawn[i]->mob);
    }

    defaultStateInvokers.clear();
  }

  MobData* GetNextMob() {
    this->nextReady = false;
    MobData* data = *(iter);
    iter++;
    pixelStateInvokers[data->index](data->mob);
    return data;
  }

  template<class CustomSpawnPolicy>
  Mob* Spawn(int tileX, int tileY);
};

template<class CustomSpawnPolicy>
Mob* Mob::Spawn(int tileX, int tileY) {
  // assert that tileX and tileY exist in field, otherwise abort
  assert(tileX >= 1 && tileX <= field->GetWidth() && tileY >= 1 && tileY <= field->GetHeight());

  MobData* data = new MobData();
  
  CustomSpawnPolicy* spawner = new CustomSpawnPolicy(*this);

  data->mob = spawner->GetSpawned();
  data->tileX = tileX;
  data->tileY = tileY;
  data->index = (unsigned)spawn.size();

  pixelStateInvokers.push_back(std::move(spawner->GetIntroCallback()));
  defaultStateInvokers.push_back(std::move(spawner->GetReadyCallback()));

  delete spawner;

  spawn.push_back(data);

  iter = spawn.begin();

  return this;
}
