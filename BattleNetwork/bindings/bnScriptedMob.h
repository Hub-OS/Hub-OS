/*! \brief Loads mob data from a lua script
 */
#pragma once
#ifdef BN_MOD_SUPPORT
#include <sol/sol.hpp>

#include "../bnMobFactory.h"
#include "../bnMob.h"
#include "../bnResourceHandle.h"
#include "bnScriptedCharacter.h"

class ScriptedMob : public MobFactory, public ResourceHandle
{
private:
  sol::state& script;
  Mob* mob{ nullptr }; //!< ptr for scripts to access
public:
  // ScriptedSpawner wrapper for scripted mobs...
  class ScriptedSpawner : public Mob::Spawner<ScriptedCharacter> {
  public:
    ScriptedSpawner(sol::state& script, const std::string& path);
    ~ScriptedSpawner();

    void SpawnAt(int x, int y);
  };

  ScriptedMob(Field* field, sol::state& script);
  ~ScriptedMob();

  /**
   * @brief Builds and returns the generated mob
   * @return Mob*
   */
  Mob* Build();
  Field* GetField();

  /**
  * @brief Creates a spawner object that loads a scripted or built-in character by its Fully Qualified Names (FQN) 
  * @param fqn String. The name of the character stored in script cache. Use `BuiltIns.NAME` prefix for built-in characters.
  * @preconditions The mob `load_script` function should never throw an exception prior to using this function.
  */
  ScriptedSpawner CreateSpawner(const std::string& fqn);

  void SetBackground(const std::string& bgTexturePath, const std::string& animPath, float velx, float vely);
  void StreamMusic(const std::string& path);
};
#endif