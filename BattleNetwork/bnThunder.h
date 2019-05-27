<<<<<<< HEAD
=======
/*! \brief Thunder crawls slowly in the field until it hits the nearest enemy
 * 
 * Thunder will apply stun damage on the enemy it hits. Every update loop,
 * it queries the closest enemy on the field and moves in that direction
 * towards the target.
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Thunder : public Spell {
protected:
  Animation animation;
  double elapsed;
<<<<<<< HEAD
  sf::Time timeout;
  Entity* target;
=======
  sf::Time timeout; /**< When time is up, thunder is removed from play */
  Entity* target; /**< The current enemy to approach */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

public:
  Thunder(Field* _field, Team _team);
  virtual ~Thunder(void);
<<<<<<< HEAD
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void Update(float _elapsed);
=======
  
  /**
   * @brief Thunder can always move to a tile no matter the conditions
   * @param tile
   * @return Always returns true
   */
  virtual bool CanMoveTo(Battle::Tile* tile);
  
  /**
   * @brief If target is null, query the field for all character enemies and track the closest one
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Attacks enemy entities. If succesful, thunder is deleted.
   * @param _entity character to attack
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Attack(Character* _entity);
}; 
