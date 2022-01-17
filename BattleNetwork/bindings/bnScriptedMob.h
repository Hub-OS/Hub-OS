/*! \brief Loads mob data from a lua script
 */
#pragma once
#ifdef BN_MOD_SUPPORT
#include <sol/sol.hpp>

#include "bnScriptedCharacter.h"
#include "../bnMobFactory.h"
#include "../bnMob.h"
#include "../bnResourceHandle.h"
#include "../bnPixelInState.h"

class ScriptedMob : public MobFactory, public ResourceHandle
{
private:
  sol::state& script;
  Mob* mob{ nullptr }; //!< ptr for scripts to access
  std::shared_ptr<Field> field{ nullptr };

public:
  // ScriptedSpawner wrapper for scripted mobs...
  class ScriptedSpawner  {
    Mob* mob{ nullptr };
    std::unique_ptr<Mob::Spawner<ScriptedCharacter>> scriptedSpawner;
    std::function<std::shared_ptr<Character>()> constructor;
    std::function<void(std::shared_ptr<Character>)> pixelStateInvoker, defaultStateInvoker;
    Character::Rank rank{};

  public:
    ScriptedSpawner() = default;
    ScriptedSpawner(sol::state& script, const std::filesystem::path& path, Character::Rank rank);

    template<typename BuiltInCharacter>
    void UseBuiltInType(Character::Rank rank);

    std::shared_ptr<Mob::Mutator> SpawnAt(int x, int y);
    void SetMob(Mob* mob);
  };

  ScriptedMob(sol::state& script);

  /**
   * @brief Builds and returns the generated mob
   * @return Mob*
   */
  Mob* Build(std::shared_ptr<Field> field, const std::string& dataString = "");
  std::shared_ptr<Field> GetField();
  void EnableFreedomMission(uint8_t turnCount, bool playerCanFlip);
  /**
  * @brief Creates a spawner object that loads a scripted or built-in character by its Fully Qualified Names (FQN)
  * @param fqn String. The name of the character stored in script cache. Use `BuiltIns.NAME` prefix for built-in characters.
  * @preconditions The mob `load_script` function should never throw an exception prior to using this function.
  */
  ScriptedSpawner CreateSpawner(const std::string& namespaceId, const std::string& fqn, Character::Rank rank);

  void SetBackground(const std::filesystem::path& bgTexturePath, const std::filesystem::path& animPath, float velx, float vely);
  void StreamMusic(const std::filesystem::path& path, long long startMs, long long endMs);
  void SpawnPlayer(unsigned playerNum, int tileX, int tileY);
};

template<typename BuiltInCharacter>
void ScriptedMob::ScriptedSpawner::UseBuiltInType(Character::Rank rank) {
  this->constructor = [rank] {
    return std::make_shared<BuiltInCharacter>(rank);
  };

  // NOTE: the difference between this invoker and the purely C++ one
  //       is we do not have the choice to change our intro state
  //       when using built-in characters from Lua
  Mob* mob = this->mob;
  this->pixelStateInvoker = [mob](std::shared_ptr<Character> character) {
    auto onFinish = [mob]() { mob->FlagNextReady(); };

    BuiltInCharacter* enemy = static_cast<BuiltInCharacter*>(character.get());

    if (enemy) {
      if constexpr (std::is_base_of<AI<BuiltInCharacter>, BuiltInCharacter>::value) {
        enemy->template ChangeState<PixelInState<BuiltInCharacter>>(onFinish);
      }
      else {
        enemy->template InterruptState<PixelInState<BuiltInCharacter>>(onFinish);
      }
    }
  };

  this->defaultStateInvoker = [](std::shared_ptr<Character> character) {
    auto enemy = static_cast<BuiltInCharacter*>(character.get());
    if (enemy) { enemy->InvokeDefaultState(); }
  };
}
#endif
