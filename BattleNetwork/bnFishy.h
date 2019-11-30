#pragma once
#include "bnObstacle.h"
#include "bnAnimation.h"

/**
 * @class Fishy
 * @author mav
 * @date 04/05/19
 * @brief Fishy attack flies across the screen, dealing damage along the way
 * 
 * Fishy can take damage and delegate the attack to the user of the attack
 */
class Fishy : public Obstacle {
protected:
  sf::Sprite fishy;
  double speed;
  bool hit;
public:
  Fishy(Field* _field, Team _team, double speed = 1.0);

  virtual ~Fishy();
  
  /**
   * @brief Fishy moves over all tiles
   * @param tile the tile to check
   * @return true
   */
  virtual bool CanMoveTo(Battle::Tile* tile);
  
  /**
   * @brief Slides across screen attacking tiles. At end of field, deletes.
   * @param _elapsed in second
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Deal flinch and recoil damage
   * @param _entity the character to attack
   */
  virtual void Attack(Character* _entity);

  /**
 * @brief At this time, fishy absorbs all attacks
 * @param props hitbox information
 * @return true
 */
  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete() { ; }
  virtual const float GetHitHeight() const { return 0; }
};
