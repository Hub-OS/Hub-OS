#pragma once
#include "bnComponent.h"
#include "bnCharacter.h"
#include "bnBattleItem.h"
#include "bnBackground.h"
#include "bnField.h"
#include <vector>
#include <map>
#include <stdexcept>

/*! \brief Manages spawning and deleting the enemy mob and delivers a reward based on rank */
class Mob
{
public:
  /*! \brief Spawn info data object */
  struct MobData {
    Character* mob; /*!< The character to spawn */
    int tileX; /*!< The tile column to spawn on */
    int tileY; /*!< The tile row to spawn on */
    unsigned index; /*!< this character's spawn order */
  };

private:
  std::vector<Component*> components; /*!< Components to inject into the battle scene */
  std::vector<MobData*> spawn; /*!< The enemies to spawn and manage */
  std::vector<MobData*>::iterator iter; /*!< Mobdata iterator */
  std::vector<std::function<void(Character*)>> defaultStateInvokers; /*!< Invoke the character's default state from the spawn policy */
  std::vector<std::function<void(Character*)>> pixelStateInvokers; /*!< Invoke the character's intro tate from the spawn policy */
  std::multimap<int, BattleItem> rewards; /*!< All possible rewards for this mob by rank */
  bool nextReady; /*!< Signal if mob is ready to spawn the next character */
  Field* field; /*!< The field to play on */
  bool isBoss; /*!< Flag to change rank and music */
  std::string music; /*!< Override with custom music */
  Background* background; /*!< Override with custom background */
public:

  /**
   * @brief constructor
   */
  Mob(Field* _field) {
    nextReady = true;
    field = _field;
    isBoss = false;
    iter = spawn.end();
    background = nullptr;
  }

  /**
   * @brief Deletes memory and cleanups all enemies
   */
  ~Mob() {
    Cleanup();
  }

  /**
   * @brief Register a reward for this mob. 
   * @param rank Cap ranks between 1-10 and where 11 is Rank S
   * @param item The item to reward with
   */
  void RegisterRankedReward(int rank, BattleItem item) {
    rank = std::max(1, rank);
    rank = std::min(11, rank);

    rewards.insert(std::make_pair(rank, item));
  }

  /**
   * @brief Find all possible rewards based on this score
   * @param score generated from BattleRewards class 
   * @return Randomly chooses possible ranked reward or nullptr if none 
   */
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

  /**
   * @brief Delete the mobdata objects and all owned components
   */
  void Cleanup() {
    for (int i = 0; i < spawn.size(); i++) {
      //delete spawn[i]->mob;
      delete spawn[i];
    }

    field = nullptr;
    spawn.clear();

    for (Component* c : components) {
      delete c;
    }

    components.clear();
  }

  /**
   * @brief Useful for debugging: kill all enemies on screen
   */
  void KillSwitch() {
    for (int i = 0; i < spawn.size(); i++) {
      spawn[i]->mob->SetHealth(0);
    }
  }

  /**
   * @brief Get the field
   * @return Field*
   */
  Field* GetField() {
    return field;
  }

  /**
   * @brief Get the count of enemies in battle
   * @return const int size
   */
  const int GetMobCount() {
    return (int)spawn.size();
  }

  void Forget(Character& character) {
    for (auto iter = spawn.begin(); iter != spawn.end(); iter++) {
      if ((*iter)->mob == &character) {
        spawn.erase(iter);
        break; // done
      }
    }

    iter = spawn.end(); // iterator needs to point to new end so IsSpawningDone() is valid
  }

  /**
   * @brief Get the count of remaining enemies in battle
   * 
   * If health is below or equal to zero, they are skipped
   * 
   * @return const int 
   */
  const int GetRemainingMobCount() {
    return int(spawn.size());
  }

  /**
   * @brief Toggle boss flag. Changes scoring system and music.
   */
  void ToggleBossFlag() {
    isBoss = !isBoss;
  }

  /**
   * @brief Query if boss battle 
   * @return true if isBoss is true, otherwise false
   */
  bool IsBoss() {
    return isBoss;
  }

  /**
   * @brief Set a custom background
   * @param background
   */
  void SetBackground(Background* background) {
    this->background = background;
  }

  /**
   * @brief Get the background object
   * @return Background*
   */
  Background* GetBackground() {
    return this->background;
  }

  /**
   * @brief The battle scene will load this custom music
   * @param path path relative to the app 
   */
  void StreamCustomMusic(const std::string path) {
    this->music = path;
  }

  /**
   * @brief Checks if custom music path was set
   * @return true if music string length is > 0
   */
  bool HasCustomMusicPath() {
    return this->music.length() > 0;
  }

  /**
   * @brief Gets the custom music path
   * @return const std::string
   */
  const std::string GetCustomMusicPath() const {
    return this->music;
  }

  /**
   * @brief Gets the mob at spawn index
   * @param index spawn index
   * @return const Character& 
   * @throws std::runtime_error if index is not in range
   */
  const Character& GetMobAt(int index) {
    if (index < 0 || index >= spawn.size()) {
      throw new std::runtime_error(std::string("Invalid index range for Mob::GetMobAt()"));
    }
    return *spawn[index]->mob;
  }

  /**
   * @brief Query if the next mob is ready to be spawned
   * @return true if flag is set and the previous spawning step is ongoing, otherwise false
   */
  const bool NextMobReady() {
    return (nextReady && !IsSpawningDone());
  }

  /**
   * @brief Query if the spawning step is finishes
   * @return if spawn iterator is at the end and the last character has spawned, return true
   */
  const bool IsSpawningDone() {
    return (iter == spawn.end() && nextReady);
  }

  /**
   * @brief Query if all the enemies have been deleted
   * @return true if all mobs are marked IsDeleted()
   */
  const bool IsCleared() {
    for (int i = 0; i < (int)spawn.size(); i++) {
      if (!spawn[i]->mob->IsDeleted()) {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Used in state ivokers to flag the mob to spawn the next piece
   * 
   * TODO: If possible hide this step from spawn policies. It's not the spawn policies direct responsibility 
   * and is always required.
   */
  void FlagNextReady() {
    this->nextReady = true;
  }

  /**
   * @brief Changes all enemies state to their default state
   */
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

  /**
   * @brief Get the next unspawned enemy
   * 
   * Spawns the mob and changes its state to PixelInState<>
   * @return MobData*
   */
  MobData* GetNextMob() {
    if (iter == spawn.end()) return nullptr;

    this->nextReady = false;
    MobData* data = *(iter);
    iter++;
    pixelStateInvokers[data->index](data->mob);
    return data;
  }

  /**
   * @brief Spawn an enemy with a custom spawn step
   * @param tileX colum to spawn the enemy on
   * @param tileY row to spawn the enemy on
   * @return Mob* to chain
   */
  template<class CustomSpawnPolicy>
  Mob* Spawn(int tileX, int tileY);
};

template<class CustomSpawnPolicy>
Mob* Mob::Spawn(int tileX, int tileY) {
  // assert that tileX and tileY exist in field, otherwise abort
  assert(tileX >= 1 && tileX <= field->GetWidth() && tileY >= 1 && tileY <= field->GetHeight());

  // Create a new enemy spawn data object
  MobData* data = new MobData();
  
  // Use a custom spawn policy
  CustomSpawnPolicy* spawner = new CustomSpawnPolicy(*this);

  // Assign the enemy to the spawn data object
  data->mob = spawner->GetSpawned();
  data->tileX = tileX;
  data->tileY = tileY;
  data->index = (unsigned)spawn.size();

  // Use the intro and default state steps provided by the policies
  pixelStateInvokers.push_back(std::move(spawner->GetIntroCallback()));
  defaultStateInvokers.push_back(std::move(spawner->GetReadyCallback()));

  // Delete the policy
  delete spawner;

  // Add the mob spawn data to our list of enemies to spawn
  spawn.push_back(data);

  // Update the iterator so that its valid
  iter = spawn.begin();

  // Return Mob* to chain
  return this;
}
