#ifdef BN_MOD_SUPPORT
#include "bnScriptedDefenseRule.h"
#include "bnWeakWrapper.h"
#include "../bnSolHelpers.h"
#include "../bnLogger.h"

ScriptedDefenseRule::ScriptedDefenseRule(const Priority level, const DefenseOrder& order) : 
    DefenseRule(level, order)
{
}

ScriptedDefenseRule::~ScriptedDefenseRule() { }

Hit::Properties& ScriptedDefenseRule::FilterStatuses(Hit::Properties& statuses)
{
  if (entries["filter_statuses_func"].valid()) 
  {
    // create a copy to protect against a scripter holding on to this variable for too long
    auto statusCopy = statuses;

    auto result = CallLuaFunctionExpectingValue<Hit::Properties>(entries, "filter_statuses_func", statusCopy);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
      return statuses;
    }

    // overwrite with the copy
    statuses = result.value();
  }

  return statuses;
}

void ScriptedDefenseRule::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) {
  if (entries["can_block_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "can_block_func", &judge, WeakWrapper(attacker), WeakWrapper(owner));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

#endif