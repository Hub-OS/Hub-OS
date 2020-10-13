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
  bool nextReady{ true }; /*!< Signal if mob is ready to spawn the next character */
  Field* field{ nullptr }; /*!< The field to play on */
  bool isBoss{ false }; /*!< Flag to change rank and music */
  std::string music; /*!< Override with custom music */
  Background* background; /*!< Override with custom background */

public:

  /**
   * @brief constructor
   */
  Mob(Field* _field);

  /**
   * @brief Deletes memory and cleanups all enemies
   */
  ~Mob();

  /**
   * @brief Register a reward for this mob. 
   * @param rank Cap ranks between 1-10 and where 11 is Rank S
   * @param item The item to reward with
   */
  void RegisterRankedReward(int rank, BattleItem item);

  /**
   * @brief Find all possible rewards based on this score
   * @param score generated from BattleRewards class 
   * @return Randomly chooses possible ranked reward or nullptr if none 
   */
  BattleItem* GetRankedReward(int score);

  /**
   * @brief Delete the mobdata objects and all owned components
   */
  void Cleanup();

  /**
   * @brief Useful for debugging: kill all enemies on screen
   */
  void KillSwitch();

  /**
   * @brief Get the field
   * @return Field*
   */
  Field* GetField();

  /**
   * @brief Get the count of enemies in battle
   * @return const int size
   */
  const int GetMobCount();

  void Forget(Character& character);

  /**
   * @brief Get the count of remaining enemies in battle
   * 
   * If health is below or equal to zero, they are skipped
   * 
   * @return const int 
   */
  const int GetRemainingMobCount();

  /**
   * @brief Toggle boss flag. Changes scoring system and music.
   */
  void ToggleBossFlag();

  /**
   * @brief Query if boss battle 
   * @return true if isBoss is true, otherwise false
   */
  bool IsBoss();

  /**
   * @brief Set a custom background
   * @param background
   */
  void SetBackground(Background* background);

  /**
   * @brief Get the background object
   * @return Background*
   */
  Background* GetBackground();

  /**
   * @brief The battle scene will load this custom music
   * @param path path relative to the app 
   */
  void StreamCustomMusic(const std::string path);

  /**
   * @brief Checks if custom music path was set
   * @return true if music string length is > 0
   */
  bool HasCustomMusicPath();

  /**
   * @brief Gets the custom music path
   * @return const std::string
   */
  const std::string GetCustomMusicPath() const;

  /**
   * @brief Gets the mob at spawn index
   * @param index spawn index
   * @return const Character& 
   * @throws std::runtime_error if index is not in range
   */
  const Character& GetMobAt(int index);

  /**
   * @brief Query if the next mob is ready to be spawned
   * @return true if flag is set and the previous spawning step is ongoing, otherwise false
   */
  const bool NextMobReady();

  /**
   * @brief Query if the spawning step is finishes
   * @return if spawn iterator is at the end and the last character has spawned, return true
   */
  const bool IsSpawningDone();

  /**
   * @brief Query if all the enemies have been deleted
   * @return true if all mobs are marked for removal from play via WillRemoveLater() == true
   */
  const bool IsCleared();

  /**
   * @brief Used in state ivokers to flag the mob to spawn the next piece
   * 
   * TODO: If possible hide this step from spawn policies. It's not the spawn policies direct responsibility 
   * and is always required.
   */
  void FlagNextReady();

  /**
   * @brief Changes all enemies state to their default state
   */
  void DefaultState();

  // todo: take both of these out
  void DelegateComponent(Component* component);
  std::vector<Component*> GetComponents();

  /**
   * @brief Get the next unspawned enemy
   * 
   * Spawns the mob and changes its state to PixelInState<>
   * @return MobData*
   */
  MobData* GetNextMob();

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
