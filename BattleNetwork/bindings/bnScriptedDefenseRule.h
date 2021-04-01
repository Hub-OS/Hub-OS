#ifdef BN_MOD_SUPPORT

#pragma once
#include "../bnDefenseRule.h"

class ScriptedDefenseRule final : public DefenseRule {

public:
  /**
  * @brief Constructs a defense rule with a priority level
  */
  ScriptedDefenseRule(const Priority level, const DefenseOrder& order);

  /**
  * @brief Deconstructor
  */
  ~ScriptedDefenseRule();

  /**
   * @brief Filters status effects before applying them to character. e.g. SuperArmor
   * @return reference to modified input statuses
   */
  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  
  /**
    * @brief Returns false if spell passes through this defense, true if defense prevents it
    */
  void CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner) override;

  // configurable callback functions from Lua
  std::function<Hit::Properties&(Hit::Properties&)> filterStatusesCallback;
  std::function<void(DefenseFrameStateJudge&, Spell&, Character&)> canBlockCallback;
};

#endif