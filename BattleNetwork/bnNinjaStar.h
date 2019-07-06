/*! \brief Ninja star strikes from above dealing 100 units of impact damage
 */

#pragma once
#include "bnSpell.h"
#include "bnTile.h"

class NinjaStar : public Spell {
private:
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
  virtual void OnUpdate(float _elapsed);
  
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
  virtual void Attack(Character* _entity);
}; 