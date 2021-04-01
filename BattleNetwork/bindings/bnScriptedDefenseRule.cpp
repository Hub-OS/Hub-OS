#ifdef BN_MOD_SUPPORT
#include "bnScriptedDefenseRule.h"

ScriptedDefenseRule::ScriptedDefenseRule(const Priority level, const DefenseOrder& order) : 
    DefenseRule(level, order)
{
}

ScriptedDefenseRule::~ScriptedDefenseRule() { }

Hit::Properties& ScriptedDefenseRule::FilterStatuses(Hit::Properties& statuses)
{
  if(filterStatusesCallback) {
      return filterStatusesCallback(statuses);
  }

  return statuses;
}

void ScriptedDefenseRule::CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner) {
    if(canBlockCallback) {
        canBlockCallback(judge, in, owner);
    }
}

#endif