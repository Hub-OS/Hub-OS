#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseAura
 * @author mav
 * @date 05/05/19
 * @brief Used with the AuraComponent which adds a rule to prevent all direct damage
 * 
 * You can use this defense rule for other reasons and create new cards that
 * do new things
 */
class DefenseAura : public DefenseRule {
public:
  typedef std::function<void(Spell*in, Character*owner)> Callback;
  
private:
  Callback callback;
  
public:
  DefenseAura(const Callback& callback);
  DefenseAura();
  
  ~DefenseAura();

  /**
   * @brief Aura defense rules never let anything through
   * @param in the attack
   * @param owner the character the rule is attached to (this)
   * @return true if hitbox is impact
   */
  const bool CanBlock(DefenseResolutionArbiter& arbiter, Spell& in, Character& owner) override;
};
