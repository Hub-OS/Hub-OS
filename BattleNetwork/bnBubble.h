#pragma once
#include "bnObstacle.h"
#include "bnAnimation.h"

/**
 * @class Bubble
 * @author mav
 * @date 05/05/19
 * @brief Bubbles move to the other side of the field, entrapping characters that makes contact with them
 * 
 * Can be popped by any amount of impact damage
 */
class Bubble : public Obstacle {
protected:
  Animation animation;
  double speed;
  bool hit;
public:
  Bubble(Field* _field, Team _team, double speed = 1.0);
  virtual ~Bubble();
  
  /**
   * @brief bubble can move to any tile no matter the state
   * @param tile
   * @return true
   */
  virtual bool CanMoveTo(Battle::Tile* tile);
  
  /**
   * @brief Continues to slide until it reaches the end of its path
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief If an obstacle, deals damage and pops. If character, tries adding a bubble trap component and pops.
   * @param _entity
   */
  virtual void Attack(Character* _entity);
  
  /**
   * @brief Always gets hit by impact, pops bubble, and plays effect
   * @param props
   * @return true
   */
  virtual const bool Hit( Hit::Properties props);

  virtual const bool OnHit(Hit::Properties props) { return true; }
  virtual void OnDelete() { ; }
  virtual const float GetHitHeight() const { return  0; }
};