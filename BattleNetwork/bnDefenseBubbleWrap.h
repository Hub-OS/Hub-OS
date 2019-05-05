#pragma once
#include <functional>
#include "bnDefenseRule.h"

/**
 * @class DefenseBubbleWrap
 * @author mav
 * @date 05/05/19
 * @file bnDefenseBubbleWrap.h
 * @brief Used by BubbleComponent to spawn hitboxes when bubble is hit
 * 
 * Allows all attacks to hit and passthrough
 */
class DefenseBubbleWrap : public DefenseRule {
public:
  DefenseBubbleWrap();

  virtual ~DefenseBubbleWrap();
  
  /**
   * @brief Allows all attacks to hit and pass 
   * @param in the attack spell
   * @param owner the character this is attached to
   * @return false
   */
  virtual const bool Check(Spell* in, Character* owner);
};
