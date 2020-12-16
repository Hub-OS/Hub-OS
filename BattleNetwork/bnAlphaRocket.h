#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

class AlphaRocket : public Obstacle {
protected:
  AnimationComponent* animation;
  double speed;
public:

  AlphaRocket(Field* _field, Team _team);

  /**
   * @brief deconstructor
   */
  virtual ~AlphaRocket();

  /**
   * @brief AlphaRocket flies through the air
   * @param tile
   * @return true
   */
  virtual bool CanMoveTo(Battle::Tile* tile);

  /**
   * @brief Moves in one direction
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(double _elapsed);

  /**
   * @brief Deals hitbox damage, exploding, and dropping hitboxes on 3 rows and columns adjacent
   * @param _entity
   */
  virtual void Attack(Character* _entity);

  virtual void OnDelete();

  virtual const float GetHeight() const;
};