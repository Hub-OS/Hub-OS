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
  Field* field{ nullptr };

public:
  // ScriptedSpawner wrapper for scripted mobs...
  class ScriptedSpawner  {
    Mob* mob{ nullptr };
    Mob::Spawner<ScriptedCharacter>* scriptedSpawner{ nullptr };
    std::function<Character* ()> constructor;
    std::function<void(Character*)> pixelStateInvoker, defaultStateInvoker;
    Character::Rank rank{};

  public:
    ScriptedSpawner() = default;
    ScriptedSpawner(sol::state& script, const std::string& path, Character::Rank rank);
    ~ScriptedSpawner();

    template<typename BuiltInCharacter>
    void UseBuiltInType(Character::Rank rank);

    Mob::Mutator* SpawnAt(int x, int y);
    void SetMob(Mob* mob);
  };

  ScriptedMob(sol::state& script);
  ~ScriptedMob();

  /**
   * @brief Builds and returns the generated mob
   * @return Mob*
   */
  Mob* Build(Field* field);
  Field* GetField();
  void EnableFreedomMission(uint8_t turnCount);
  /**
  * @brief Creates a spawner object that loads a scripted or built-in character by its Fully Qualified Names (FQN) 
  * @param fqn String. The name of the character stored in script cache. Use `BuiltIns.NAME` prefix for built-in characters.
  * @preconditions The mob `load_script` function should never throw an exception prior to using this function.
  */
  ScriptedSpawner CreateSpawner(const std::string& fqn, Character::Rank rank);

  void SetBackground(const std::string& bgTexturePath, const std::string& animPath, float velx, float vely);
  void StreamMusic(const std::string& path, long long startMs, long long endMs);
};

template<typename BuiltInCharacter>
void ScriptedMob::ScriptedSpawner::UseBuiltInType(Character::Rank rank) {
  this->constructor = [rank] {
    return new BuiltInCharacter(rank);
  };

  // NOTE: the difference between this invoker and the purely C++ one
  //       is we do not have the choice to change our intro state
  //       when using built-in characters from Lua
  Mob* mob = this->mob;
  this->pixelStateInvoker = [mob](Character* character) {
    auto onFinish = [mob]() { mob->FlagNextReady(); };

    BuiltInCharacter* enemy = static_cast<BuiltInCharacter*>(character);

    if (enemy) {
      if constexpr (std::is_base_of<AI<BuiltInCharacter>, BuiltInCharacter>::value) {
        enemy->template ChangeState<PixelInState<BuiltInCharacter>>(onFinish);
      }
      else {
        enemy->template InterruptState<PixelInState<BuiltInCharacter>>(onFinish);
      }
    }
  };

  this->defaultStateInvoker = [](Character * character) {
    BuiltInCharacter* enemy = static_cast<BuiltInCharacter*>(character);
    if (enemy) { enemy->InvokeDefaultState(); }
  };
}
#endif