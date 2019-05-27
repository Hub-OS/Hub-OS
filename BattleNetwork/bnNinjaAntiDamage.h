<<<<<<< HEAD
=======
/*! \brief Adds and removes defense rule for antidamage checks.
 *         Spawns ninja stars in retaliation
 */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

<<<<<<< HEAD
/* 
   Adds and removes defense rule for antidamage checks.
   Spawns ninja stars in retaliation
*/

=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class DefenseRule;

class NinjaAntiDamage : public Component {
private:
<<<<<<< HEAD
  DefenseRule* defense;

public:
  NinjaAntiDamage(Entity* owner);
  ~NinjaAntiDamage();

  virtual void Update(float _elapsed);
=======
  DefenseRule* defense; /*!< Adds defense rule to the owner */

public:
  /**
   * @brief Builds a defense rule for anti damage with a callback to spawn ninja stars
   */
  NinjaAntiDamage(Entity* owner);
  
  /**
   * @brief delete defense rule pointer
   */
  ~NinjaAntiDamage();

  /**
   * @brief Does nothing
   * @param _elapsed
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Not injected into battle scene
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Inject(BattleScene&);
};
