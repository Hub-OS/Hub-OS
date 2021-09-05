#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseInvis
 * @author mav
 * @date 05/05/19
 * @brief Invis is a rule that prevents all tangible attacks from passing unless it has pierce ability
 *
 * This is used by the Invis card's component
 */
class DefenseInvis : public DefenseRule {
public:
  DefenseInvis();
  ~DefenseInvis();

  /**
   * @brief If the attack does pierce damage, the defense fails
   * @param in attack spell
   * @param owner the character with Invis defense (this) added
   */
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};