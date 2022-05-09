#pragma once
#include "bnAI.h"
#include "bnComponent.h"
#include "bnCharacter.h"
#include "bnBattleItem.h"
#include "bnBackground.h"
#include "bnField.h"
#include "bnMobHealthUI.h"
#include "bnTile.h"
#include "stx/tuple.h"

#include <tuple>
#include <vector>
#include <map>
#include <stdexcept>
#include <type_traits>

/*! \brief Manages spawning and deleting the enemy mob and delivers a reward based on rank */
class Mob
{
public:
  /*! \brief Spawn info data object */
  struct SpawnData {
    std::shared_ptr<Character> character{ nullptr }; /*!< The character to spawn */
    int tileX{}; /*!< The tile column to spawn on */
    int tileY{}; /*!< The tile row to spawn on */
    unsigned index{}; /*!< this character's spawn order */
  };

  struct PlayerSpawnData {
    int tileX{};
    int tileY{};
  };

  class Mutator {
    friend class Mob;

    std::unique_ptr<SpawnData> data;
    std::vector<std::function<void(std::shared_ptr<Character> in)>> mutations;

    std::unique_ptr<SpawnData> GetSpawned() {
      for (auto&& m : mutations) {
        m(data->character);
      }

      return std::move(data);
    }

  public:
    Mutator(std::unique_ptr<SpawnData> data) : data(std::move(data)) {

    }

    void Mutate(const std::function<void(std::shared_ptr<Character> in)>& mutation) {
      mutations.push_back(mutation);
    }
  };

private:
  uint8_t turnLimit{};
  size_t spawnIndex{}; /*!< Current character to spawn in the spawn loop*/
  long long startMs{-1}, endMs{-1};
  bool nextReady{ true }; /*!< Signal if mob is ready to spawn the next character */
  bool isBoss{ false }; /*!< Flag to change rank and music */
  bool isFreedomMission{ false }, playerCanFlip{ false };
  std::filesystem::path music; /*!< Override with custom music */
  std::vector<std::shared_ptr<Mutator>> spawn; /*!< The enemies to spawn and manage */
  std::vector<std::shared_ptr<Character>> tracked; /*! Enemies that may or may not be spawned through the mob class but need to be considered */
  std::vector<std::function<void(std::shared_ptr<Character>)>> defaultStateInvokers; /*!< Invoke the character's default state from the spawn policy */
  std::vector<std::function<void(std::shared_ptr<Character>)>> pixelStateInvokers; /*!< Invoke the character's intro tate from the spawn policy */
  std::map<unsigned, PlayerSpawnData> playerSpawnPoints; /*!< Player index spawns at the desired target location */
  std::multimap<int, BattleItem> rewards; /*!< All possible rewards for this mob by rank */
  std::shared_ptr<Field> field{ nullptr }; /*!< The field to play on */
  std::shared_ptr<Background> background; /*!< Override with custom background */

  sf::Vector2i panelSpacing; /*!< Override with custom panel spacing */
  sf::Vector2i panelStartPos; /*!< Override with custom panel startPos */
  std::array<std::shared_ptr<sf::Texture>, 3> panelTextures; /*!< Override with custom panel texture */
  Animation panelAnimation; /*!< Required when overriding custom panel texture */
public:
  friend class ScriptedMob;

  // forward decl
  template<class ClassType> class Spawner;

  /**
   * @brief constructor
   */
  Mob(std::shared_ptr<Field> _field);

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
   * @return std::shared_ptr<Field>
   */
  std::shared_ptr<Field> GetField();

  /**
   * @brief Get the count of enemies in battle
   * @return const int size
   */
  const int GetMobCount();

  /**
   * @brief removes a character from the mob list
   *
   * When the list is empty, the mob has been cleared
   * We wish to forget about them because the Field class
   * handles deletion of entities that have spawned
   *
   * @see Field
   */
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
   * @brief Toggle boss flag to true. Changes scoring system and music.
   */
  void EnableBossBattle();

  /**
   * @brief Query if boss battle
   * @return true if isBoss is true, otherwise false
   */
  bool IsBoss();

  /**
  * @brief Query if freedom mission requested
  */
  bool IsFreedomMission();
  bool PlayerCanFlip();

  bool HasPlayerSpawnPoint(unsigned playerNum);
  PlayerSpawnData& GetPlayerSpawnPoint(unsigned playerNum);

  /*
  * @brief Query the number of allowed turns. Zero means no limit.
  */
  uint8_t GetTurnLimit();

  /**
   * @brief Set a custom background
   * @param background
   */
  void SetBackground(const std::shared_ptr<Background> background);

  /**
   * @brief Get the background object
   * @return std::shared_ptr<Background>
   */
  std::shared_ptr<Background> GetBackground();

  /**
  * @brief Set a custom panel texture and its corresponding animation file
  * @param 3 textures
  * @param animation
  * @param spacingX how many pixels apart horizontally tiles are spaced
  * @param spacingY how many pixels apart vertically tiles are spaced
  */
  void SetPanels(const std::array<std::shared_ptr<sf::Texture>, 3>& textures, const Animation& animation, sf::Vector2i startPos, sf::Vector2i spacing);

  /**
   * @brief Get the panel texture
   * @return std::array<std::shared_ptr<Texture>, 3>
   */
  const std::array<std::shared_ptr<Texture>, 3> GetPanelTextures();

  /**
   * @brief Get the corresponding animation to the panel texture
   * @return const Animation
   */
  const Animation GetPanelAnimation();

  const sf::Vector2i GetPanelSpacing() const;
  const sf::Vector2i GetPanelStartPos() const;

  const bool HasCustomPanels() const;

  /**
   * @brief The battle scene will load this custom music
   * @param path path relative to the app
   */
  void StreamCustomMusic(const std::filesystem::path& path, long long startMs = -1LL, long long endMs = -1LL);

  /**
   * @brief Checks if custom music path was set
   * @return true if music string length is > 0
   */
  bool HasCustomMusicPath();

  /**
   * @brief Gets the custom music path
   * @return const std::filesystem::path
   */
  const std::filesystem::path GetCustomMusicPath() const;

  /**
  * @brief Gets the music loop points
  * @return const std::array<long long, 2>. the loop start and end points in milliseconds
  */
  const std::array<long long, 2> GetLoopPoints() const;

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
   * @return true if all mobs are marked for removal from play via WillEraseEOF() == true
   */
  const bool IsCleared();

  /**
   * @brief Used in state ivokers to flag the mob to spawn the next piece
   *
   */
  void FlagNextReady();

  /**
   * @brief Changes all enemies state to their default state
   */
  void DefaultState();

  /**
  * @brief Specify where the player(s) should spawn in this mob
  * @param playerNum the player order e.g. player 1 spawns at tile (x,y)
  * @param tileX the column of the target tile
  * @param tileY the row of the target tile
  *
  * @warning will overwrite any existing playerNum spawn data if a previous `playerNum` entry exists
  */
  void SpawnPlayer(unsigned playerNum, int tileX, int tileY);

  /**
   * @brief Get the next unspawned enemy
   *
   * Spawns the character and changes its state to PixelInState<>
   * @return SpawnData
   */
  std::unique_ptr<SpawnData> GetNextSpawn();

  /**
   * @brief Create a spawner object to spawn typed enemies
   * @param Args... constructor arguments for deferred loading
   * @return Spawner<> object to reuse
   */
  template<class ClassType>
  Spawner<ClassType> CreateSpawner();

  // with args...
  template<class ClassType, typename... Args>
  Spawner<ClassType> CreateSpawner(Args&&...);

  void Track(std::shared_ptr<Character> character);

  bool EnablePlayerCanFlip(bool enabled);
  void EnableFreedomMission(bool enabled);
  void LimitTurnCount(uint8_t turnLimit);

};

//
// Mob::CreateSpawner() function implementations...
//

template<class ClassType>
Mob::Spawner<ClassType> Mob::CreateSpawner() {
  auto item = Mob::Spawner<ClassType>();
  item.SetMob(this);
  return item;
}

template<class ClassType, typename... Args>
Mob::Spawner<ClassType> Mob::CreateSpawner(Args&&... args) {
  auto item = Mob::Spawner<ClassType>(std::forward<decltype(args)>(args)...);
  item.SetMob(this);
  return item;
}

//
// Spawner implementation...
//

template<class ClassType>
class Mob::Spawner {
protected:
  std::function<std::shared_ptr<ClassType>()> constructor;
  Mob* mob{ nullptr };
public:
  friend class ScriptedMob;

  // ctors with zero arguments (or default args are provided)
  Spawner() {
    constructor = []() mutable {
      return std::make_shared<ClassType>();
    };
  }

  // copy ctor
  Spawner(const Spawner& rhs) : constructor(rhs.constructor), mob(rhs.mob) {}

  // c-tors with arguments (std::enable_if_t prevents matching with the copy ctor)
  template<typename... Args, typename std::enable_if_t<!std::is_constructible<Args&&..., const Spawner&>::value>* = nullptr>
  explicit Spawner(Args&&... args) {
    constructor = [tuple_args = std::make_tuple(std::forward<decltype(args)>(args)...)]() mutable->std::shared_ptr<ClassType> {
      auto character = stx::make_shared_from_tuple<ClassType>(tuple_args);
      character->Init();
      return character;
    };
  }

  // virtual deconstructor for inheritence
  virtual ~Spawner() { }

  template<class IntroState, typename... Args>
  std::shared_ptr<Mutator> SpawnAt(unsigned x, unsigned y, Args&&... args) {
    // assert that tileX and tileY exist in field
    assert(x >= 1 && x <= static_cast<unsigned>(mob->field->GetWidth())
        && y >= 1 && y <= static_cast<unsigned>(mob->field->GetHeight()));

    // Create a new enemy spawn data object
    auto data = std::make_unique<SpawnData>();
    std::shared_ptr<Character> character = constructor();

    auto ui = character->CreateComponent<MobHealthUI>(character);
    ui->Hide(); // let default state invocation reveal health

    // Assign the enemy to the spawn data object
    data->character = character;
    data->tileX = x;
    data->tileY = y;
    data->index = (unsigned)mob->spawn.size();

    mob->field->GetAt(x, y)->ReserveEntityByID(character->GetID());

    // Thinking we need to remove AI inheritence and be another component on its own
    Mob* mobPtr = this->mob;
    auto pixelStateInvoker = [mobPtr, tupleArgs = std::make_tuple(std::forward<Args>(args) ...)](std::shared_ptr<Character> character) {
      auto onFinish = [mobPtr]() { mobPtr->FlagNextReady(); };

      ClassType* enemy = static_cast<ClassType*>(character.get());

      auto fullTupleArgs = std::tuple_cat(std::make_tuple(onFinish), tupleArgs);
      using FullTupleArgsT = decltype(fullTupleArgs);

      if (enemy) {
        if constexpr (std::is_base_of<AI<ClassType>, ClassType>::value) {
          enemy->template ChangeState<IntroState, FullTupleArgsT, std::tuple_size_v<FullTupleArgsT>>(fullTupleArgs);
        }
        else {
          enemy->template InterruptState<IntroState>, FullTupleArgsT, std::tuple_size_v<FullTupleArgsT>>(fullTupleArgs);
        }
      }
    };

    auto defaultStateInvoker = [ui](std::shared_ptr<Character> character) {
      ClassType* enemy = static_cast<ClassType*>(character.get());
      if (enemy) { enemy->InvokeDefaultState(); ui->Reveal(); }
    };

    mob->pixelStateInvokers.push_back(pixelStateInvoker);
    mob->defaultStateInvokers.push_back(defaultStateInvoker);

    // Set name special font based on rank
    switch (data->character->GetRank()) {
    case Character::Rank::_2:
      data->character->SetName(data->character->GetName() + Font::v2());
      break;
    case Character::Rank::_3:
      data->character->SetName(data->character->GetName() + Font::v3());
      break;
    case Character::Rank::Rare1:
      data->character->SetName(data->character->GetName() + Font::v4());
      break;
    case Character::Rank::Rare2:
      data->character->SetName(data->character->GetName() + Font::v5());
      break;
    case Character::Rank::SP:
      data->character->SetName(data->character->GetName() + Font::sp());
      break;
    case Character::Rank::EX:
      data->character->SetName(data->character->GetName() + Font::ex());
      break;
    case Character::Rank::NM:
      data->character->SetName(data->character->GetName() + Font::nm());
      break;
    case Character::Rank::RV:
      data->character->SetName(data->character->GetName() + Font::rv());
      break;
    case Character::Rank::DS:
      data->character->SetName(data->character->GetName() + Font::ds());
      break;
    case Character::Rank::Alpha:
      data->character->SetName(data->character->GetName() + Font::alpha());
      break;
    case Character::Rank::Beta:
      data->character->SetName(data->character->GetName() + Font::beta());
      break;
    case Character::Rank::Omega:
      data->character->SetName(data->character->GetName() + Font::omega());
      break;
    case Character::Rank::Sigma:
      data->character->SetName(data->character->GetName() + Font::sigma());
      break;
    }

    // Add the character to our list of enemies to spawn
    mob->tracked.push_back(data->character);

    // Return a mutator to change some spawn info
    auto mutator = std::make_shared<Mutator>(std::move(data));
    mob->spawn.push_back(mutator);

    return mutator;
  }

  void SetMob(Mob* ptr) {
    this->mob = ptr;
  }
};
