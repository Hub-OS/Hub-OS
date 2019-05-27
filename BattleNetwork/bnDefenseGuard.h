#pragma once
#include <functional>
#include "bnDefenseRule.h"

<<<<<<< HEAD
=======
/**
 * @class DefenseGuard
 * @author mav
 * @date 05/05/19
 * @brief If an attack does not have a breaking hit property, fires a callback
 * 
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class DefenseGuard : public DefenseRule {
public:
  typedef std::function<void(Spell* in, Character* owner)> Callback;

private:
  Callback callback;

public:
  DefenseGuard(Callback callback);

  virtual ~DefenseGuard();

<<<<<<< HEAD
=======
  /**
   * @brief Check for breaking properties
   * @param in the attack
   * @param owner the character this is attached to
   * @return Returns true if spell does not have breaking properties, false otherwise
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual const bool Check(Spell* in, Character* owner);
};
