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
  bool changed{ false };
  AnimationComponent* animation{ nullptr };
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
  ~NinjaStar();

  /**
   * @brief Interpol. from start to tile attacking entites on the tile when anim finishes
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;
  
  /**
   * @brief Does not move across tiles
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
  
  /**
   * @brief Attacks entity
   * @param _entity to deal hitbox damage to
   */
  void Attack(Character* _entity) override;

  /**
  * @brief Does nothing
  */
  void OnDelete() override;
}; 