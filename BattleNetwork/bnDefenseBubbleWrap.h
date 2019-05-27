#pragma once
#include <functional>
#include "bnDefenseRule.h"

<<<<<<< HEAD
=======
/**
 * @class DefenseBubbleWrap
 * @author mav
 * @date 05/05/19
 * @brief Used by BubbleComponent to spawn hitboxes when bubble is hit
 * 
 * Allows all attacks to hit and passthrough
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class DefenseBubbleWrap : public DefenseRule {
public:
  DefenseBubbleWrap();

  virtual ~DefenseBubbleWrap();
<<<<<<< HEAD

=======
  
  /**
   * @brief Allows all attacks to hit and pass 
   * @param in the attack spell
   * @param owner the character this is attached to
   * @return false
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual const bool Check(Spell* in, Character* owner);
};
