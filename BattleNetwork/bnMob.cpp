#include "bnMob.h"

#pragma once
#include "bnComponent.h"
#include "bnCharacter.h"
#include "bnBattleItem.h"
#include "bnBackground.h"
#include "bnField.h"
#include <vector>
#include <map>
#include <stdexcept>

Mob::Mob(Field* _field) {
  nextReady = true;
  field = _field;
  isBoss = false;
  iter = spawn.end();
  background = nullptr;
}

Mob::~Mob() {
  Cleanup();
}

void Mob::RegisterRankedReward(int rank, BattleItem item) {
  rank = std::max(1, rank);
  rank = std::min(11, rank);

  rewards.insert(std::make_pair(rank, item));
}

BattleItem* Mob::GetRankedReward(int score) {
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

void Mob::Cleanup() {
  for (int i = 0; i < spawn.size(); i++) {
    delete spawn[i];
  }

  field = nullptr;
  spawn.clear();

  for (Component* c : components) {
    delete c;
  }

  components.clear();
}

void Mob::KillSwitch() {
  for (int i = 0; i < spawn.size(); i++) {
    spawn[i]->mob->SetHealth(0);
  }
}

Field* Mob::GetField() {
  return field;
}

const int Mob::GetMobCount() {
  return (int)spawn.size();
}

void Mob::Forget(Character& character) {
  if (spawn.empty()) return;

  auto forgetIter = spawn.begin();
  while(forgetIter != spawn.end()) {
    if ((*forgetIter)->mob == &character) {
      spawn.erase(forgetIter);
      break; // done
    }
    forgetIter++;
  }

  iter = spawn.end(); // iterator needs to point to new end so IsSpawningDone() is valid
}

const int Mob::GetRemainingMobCount() {
  return int(spawn.size());
}

void Mob::ToggleBossFlag() {
  isBoss = !isBoss;
}

bool Mob::IsBoss() {
  return isBoss;
}

void Mob::SetBackground(Background* background) {
  Mob::background = background;
}

Background* Mob::GetBackground() {
  return background;
}

void Mob::StreamCustomMusic(const std::string path) {
  music = path;
}

bool Mob::HasCustomMusicPath() {
  return music.length() > 0;
}

const std::string Mob::GetCustomMusicPath() const {
  return music;
}

const Character& Mob::GetMobAt(int index) {
  if (index < 0 || index >= spawn.size()) {
    throw new std::runtime_error(std::string("Invalid index range for Mob::GetMobAt()"));
  }
  return *spawn[index]->mob;
}

const bool Mob::NextMobReady() {
  return (nextReady && !IsSpawningDone());
}

const bool Mob::IsSpawningDone() {
  return ((spawn.size() == 0 || iter == spawn.end()) && nextReady);
}

const bool Mob::IsCleared() {
  for (int i = 0; i < (int)spawn.size(); i++) {
    auto mob = spawn[i]->mob;
    if (!mob->WillRemoveLater()) {
      return false;
    }
  }

  return true;
}

void Mob::FlagNextReady() {
  nextReady = true;
}

void Mob::DefaultState() {
  for (int i = 0; i < (int)defaultStateInvokers.size(); i++) {
    defaultStateInvokers[i](spawn[i]->mob);
  }

  defaultStateInvokers.clear();
}

// todo: take out
void Mob::DelegateComponent(Component* component) {
  components.push_back(component);
}

std::vector<Component*> Mob::GetComponents() { 
  return components; 
}

Mob::MobData* Mob::GetNextMob() {
  if (iter == spawn.end()) return nullptr;

  nextReady = false;
  MobData* data = *(iter);
  iter++;
  pixelStateInvokers[data->index](data->mob);
  return data;
}
