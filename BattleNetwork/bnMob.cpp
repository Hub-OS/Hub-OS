#include "bnMob.h"

#include "bnComponent.h"
#include "bnCharacter.h"
#include "bnBattleItem.h"
#include "bnBackground.h"
#include "bnField.h"
#include <vector>
#include <map>
#include <stdexcept>

Mob::Mob(std::shared_ptr<Field> _field) {
  nextReady = true;
  field = _field;
  isBoss = false;
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
  /*iter = spawn.end();
  field = nullptr;
  spawn.clear();
  components.clear();*/
}

void Mob::KillSwitch() {
  for (int i = 0; i < tracked.size(); i++) {
    tracked[i]->SetHealth(0);
  }
}

std::shared_ptr<Field> Mob::GetField() {
  return field;
}

const int Mob::GetMobCount() {
  return (int)tracked.size();
}

void Mob::Forget(Character& character) {
  if (tracked.empty()) return;

  auto forgetIter = tracked.begin();
  while(forgetIter != tracked.end()) {
    if ((*forgetIter).get() == &character) {
      tracked.erase(forgetIter);
      break; // done
    }
    forgetIter++;
  }
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

bool Mob::IsFreedomMission()
{
  return isFreedomMission;
}

bool Mob::PlayerCanFlip()
{
  return playerCanFlip;
}

bool Mob::HasPlayerSpawnPoint(unsigned playerNum)
{
  return playerSpawnPoints.find(playerNum) != playerSpawnPoints.end();
}

Mob::PlayerSpawnData& Mob::GetPlayerSpawnPoint(unsigned playerNum)
{
  return playerSpawnPoints[playerNum];
}

uint8_t Mob::GetTurnLimit()
{
  return turnLimit;
}

void Mob::SetBackground(const std::shared_ptr<Background> background) {
  Mob::background = background;
}

std::shared_ptr<Background> Mob::GetBackground() {
  return background;
}

void Mob::StreamCustomMusic(const std::filesystem::path& path, long long startMs, long long endMs) {
  music = path;
  this->startMs = startMs;
  this->endMs = endMs;
}

bool Mob::HasCustomMusicPath() {
  return !music.empty();
}

const std::filesystem::path Mob::GetCustomMusicPath() const {
  return music;
}

const std::array<long long, 2> Mob::GetLoopPoints() const
{
  return { startMs, endMs };
}

const Character& Mob::GetMobAt(int index) {
  if (index < 0 || index >= tracked.size()) {
    throw new std::runtime_error(std::string("Invalid index range for Mob::GetMobAt()"));
  }
  return *tracked[index];
}

const bool Mob::NextMobReady() {
  return (nextReady && !IsSpawningDone());
}

const bool Mob::IsSpawningDone() {
  return ((spawn.size() == 0 || spawnIndex >= spawn.size()) && nextReady);
}

const bool Mob::IsCleared() {
  return tracked.empty();
}

void Mob::FlagNextReady() {
  nextReady = true;
}

void Mob::DefaultState() {
  for (int i = 0; i < (int)defaultStateInvokers.size(); i++) {
    defaultStateInvokers[i](tracked[i]);
  }

  defaultStateInvokers.clear();
}

void Mob::SpawnPlayer(unsigned playerNum, int tileX, int tileY)
{
  playerSpawnPoints[playerNum] = { tileX, tileY };
}

std::unique_ptr<Mob::SpawnData> Mob::GetNextSpawn() {
  if (spawnIndex >= spawn.size()) return nullptr;

  nextReady = false;
  auto data = spawn[spawnIndex++]->GetSpawned();
  pixelStateInvokers[data->index](data->character);
  return data;
}

void Mob::Track(std::shared_ptr<Character> character)
{
  if (std::find(tracked.begin(), tracked.end(), character) == tracked.end()) {
    tracked.push_back(character);
  }
}

bool Mob::EnablePlayerCanFlip(bool enabled)
{
  return playerCanFlip = enabled;
}

void Mob::EnableFreedomMission(bool enabled)
{
  isFreedomMission = enabled;
}

void Mob::LimitTurnCount(uint8_t turnLimit)
{
  Mob::turnLimit = turnLimit;
}
