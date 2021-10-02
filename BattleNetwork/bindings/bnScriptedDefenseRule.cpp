#ifdef BN_MOD_SUPPORT
#include "bnScriptedDefenseRule.h"

#include "../bnLogger.h"

ScriptedDefenseRule::ScriptedDefenseRule(const Priority level, const DefenseOrder& order) : 
    DefenseRule(level, order)
{
}

ScriptedDefenseRule::~ScriptedDefenseRule() { }

Hit::Properties& ScriptedDefenseRule::FilterStatuses(Hit::Properties& statuses)
{
  if (filterStatusesCallback) {
    try {
      return filterStatusesCallback(statuses);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }

  return statuses;
}

void ScriptedDefenseRule::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) {
  if (canBlockCallback) {
    try {
      canBlockCallback(judge, attacker, owner);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

#endif