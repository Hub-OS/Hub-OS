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
  bool popping;
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
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief If an obstacle, deals damage and pops. If character, tries adding a bubble trap component and pops.
   * @param _entity
   */
  virtual void Attack(Character* _entity);

  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete();
  virtual const float GetHeight() const;
};