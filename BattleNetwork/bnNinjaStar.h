<<<<<<< HEAD
=======
/*! \brief Ninja star strikes from above dealing 100 units of impact damage
 */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnSpell.h"
#include "bnTile.h"

class NinjaStar : public Spell {
private:
<<<<<<< HEAD
  sf::Vector2f start;
  double progress;
  double duration;

public:
  NinjaStar(Field* _field, Team _team, float _duration);
  virtual ~NinjaStar(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
=======
  sf::Vector2f start; /*!< Start position to interpolate from */
  double progress; /*!< Progress of the animation */
  double duration; /*!< How quickly the animation plays */

public:

  /**
   * @brief Blue team star spawns at (480,0) Red ream spawns at (0,0)
   * @@param _team Team of ninja star
   * @param _duration of the animation
   */
  NinjaStar(Field* _field, Team _team, float _duration);
  
  /**
   * @brief deconstructor
   */
  virtual ~NinjaStar();

  /**
   * @brief Interpol. from start to tile attacking entites on the tile when anim finishes
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Does not move across tiles
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief Attacks entity
   * @param _entity to deal hitbox damage to
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Attack(Character* _entity);
}; 