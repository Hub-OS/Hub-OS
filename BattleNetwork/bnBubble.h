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

  ~Bubble();
  
  /**
   * @brief bubble can move to any tile no matter the state
   * @param tile
   * @return true
   */
  bool CanMoveTo(Battle::Tile* tile) override;
  
  /**
   * @brief Continues to slide until it reaches the end of its path
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;
  
  /**
   * @brief If an obstacle, deals damage and pops. If character, tries adding a bubble trap component and pops.
   * @param _entity
   */
  void Attack(Character* _entity) override;

  void OnCollision() override;

  const bool Bubble::UnknownTeamResolveCollision(const Spell& other) const override final;

  void OnDelete() override;
  const float GetHeight() const override;
};