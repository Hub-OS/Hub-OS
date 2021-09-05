#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseAntiDamage
 * @author mav
 * @date 05/05/19
 * @brief AntiDamage is a rule that blocks certain attacks in the attack resolution step
 * @see NinjaAntiDamage.h
 * 
 * This is used by the NinjaAntiDamage component that adds the rule to the entity
 * and spawns a NinjaStar when the callback is triggered
 * 
 * You can create more anti damage cards using this rule
 */
class DefenseAntiDamage : public DefenseRule {
public:
  typedef std::function<void(std::shared_ptr<Spell> in, std::shared_ptr<Character> owner)> Callback;

private:
  Callback callback; /*!< Runs when the antidefense is triggered */
  bool triggering{ false };

public:
  /**
   * @brief sets callback
   * @param callback
   */
  DefenseAntiDamage(const Callback& callback);

  ~DefenseAntiDamage();

  /**
   * @brief If the attack does > 10 units of impact damage, triggers the callback
   * @param in attack spell
   * @param owner the character with antidamage defense (this) added 
   */
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};
