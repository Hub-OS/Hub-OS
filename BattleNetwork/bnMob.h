#pragma once
#include "bnComponent.h"
#include "bnCharacter.h"
#include "bnMeta.h"
#include "bnBattleItem.h"
#include "bnBackground.h"
#include "bnField.h"
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
  std::vector<Component*> components;
  std::vector<MobData*> spawn;
  std::vector<MobData*>::iterator iter;
  std::vector<std::function<void(Character*)>> defaultStateInvokers;
  std::vector<std::function<void(Character*)>> pixelStateInvokers;
  std::multimap<int, BattleItem> rewards;
  bool nextReady;
  Field* field;
  bool isBoss;
  std::string music;
  Background* background;
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

    for (Component* c : components) {
      delete c;
    }

    components.clear();
  }

  Field* GetField() {
    return field;
  }

  const int GetMobCount() {
    return (int)spawn.size();
  }

  const int GetRemainingMobCount() {
    int remaining = (int)spawn.size();

    for (int i = 0; i < (int)spawn.size(); i++) {
      if (spawn[i]->mob->GetHealth() <= 0) {
        remaining--;
      }
    }

    return remaining;
  }

  void ToggleBossFlag() {
    isBoss = !isBoss;
  }

  bool IsBoss() {
    return isBoss;
  }

  void SetBackground(Background* background) {
    this->background = background;
  }

  Background* GetBackground() {
    return this->background;
  }

  void StreamCustomMusic(const std::string path) {
    this->music = path;
  }

  bool HasCustomMusicPath() {
    return this->music.length() > 0;
  }

  const std::string GetCustomMusicPath() const {
    return this->music;
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
    for (int i = 0; i < (int)spawn.size(); i++) {
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
    for (int i = 0; i < (int)defaultStateInvokers.size(); i++) {
      defaultStateInvokers[i](spawn[i]->mob);
    }

    defaultStateInvokers.clear();
  }

  // todo: take out
  void DelegateComponent(Component* component) {
    components.push_back(component);
  }

  std::vector<Component*> GetComponents() { return components; }

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
