#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseStatusGuard
 * @author mav
 * @date 11/11/2020
 * @brief stun, bubble, and freeze effects are voided
 *
 */
class DefenseStatusGuard : public DefenseRule {

public:
  DefenseStatusGuard();

  ~DefenseStatusGuard();

  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;

  /**
   * @brief Check for status properties and void them
   * @param in the attack
   * @param owner the character this is attached to
   */
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) override;
};
