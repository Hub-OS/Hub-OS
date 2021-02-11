/*! \brief Loads mob data from a lua script
 */
#pragma once
#include <sol/sol.hpp>

#include "../bnMobFactory.h"

class ScriptedMob : public MobFactory
{
  private:
  sol::state& script;
public:
  ScriptedMob(Field* field, sol::state& script);
  ~ScriptedMob();

  /**
   * @brief Builds and returns the generated mob
   * @return Mob*
   */
  Mob* Build();
};

