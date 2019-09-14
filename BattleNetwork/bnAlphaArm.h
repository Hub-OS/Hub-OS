#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/**
 * @class Alpha Arm
*/
class AlphaArm : public Obstacle {
public:
  AlphaArm(Field * _field, Team _team);
  virtual ~AlphaArm();

  virtual void OnUpdate(float _elapsed);

  virtual const bool OnHit(const Hit::Properties props);
 
  virtual void OnDelete();

  virtual bool CanMoveTo(Battle::Tile * next);

  virtual void Attack(Character* e);

  virtual const float GetHitHeight() const;

private:
  Direction startDir;
  bool hit, isMoving;
  float totalElapsed;
  SpriteSceneNode* shadow;
};
