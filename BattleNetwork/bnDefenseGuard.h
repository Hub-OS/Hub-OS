#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseGuard
 * @author mav
 * @date 05/05/19
 * @brief If an attack does not have a breaking hit property, fires a callback
 * 
 */
class DefenseGuard : public DefenseRule {
public:
  typedef std::function<void(Spell* in, Character* owner)> Callback;

private:
  Callback callback;

public:
  DefenseGuard(const Callback& callback);
  ~DefenseGuard();

  /**
   * @brief Check for breaking properties
   * @param in the attack
   * @param owner the character this is attached to
   * @return Returns true if spell does not have breaking properties, false otherwise
   */
  const bool CanBlock(DefenseResolutionArbiter& arbiter, Spell& in, Character& owner);
};
