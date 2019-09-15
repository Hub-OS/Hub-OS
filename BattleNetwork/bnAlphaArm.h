#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/**
 * @class Alpha Arm
*/
class AlphaArm : public Obstacle {
public:
  enum class Type : int {
    LEFT_IDLE, RIGHT_IDLE, LEFT_SWIPE, RIGHT_SWIPE
  } type;

  AlphaArm(Field * _field, Team _team, AlphaArm::Type type);
  virtual ~AlphaArm();

  virtual void OnUpdate(float _elapsed);

  virtual const bool OnHit(const Hit::Properties props);
 
  virtual void OnDelete();

  virtual bool CanMoveTo(Battle::Tile * next);

  virtual void Attack(Character* e);

  virtual const float GetHitHeight() const;

  const bool IsSwiping() const;

  void SyncElapsedTime(const float elapsedTime);

private:
  TileState changeState;
  Direction startDir;
  bool hit, isMoving;
  float totalElapsed;
  SpriteSceneNode* shadow;
  SpriteSceneNode* blueShadow;
  std::vector<sf::Vector2f> blueArmShadowPos;
  int blueArmShadowPosIdx;
  float blueShadowTimer; // 1/60 of a second = 1 frame
  bool isSwiping;
};
