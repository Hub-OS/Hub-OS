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
  ~AlphaArm();

  void OnUpdate(float _elapsed);

  const bool OnHit(const Hit::Properties props);
 
  void OnDelete();

  bool CanMoveTo(Battle::Tile * next);

  void Attack(Character* e);

  const float GetHeight() const;

  const bool IsSwiping() const;

  void SyncElapsedTime(const float elapsedTime);

  void LeftArmChangesTileState();
  void RightArmChangesTileTeam();

  const bool GetIsFinished() const;

private:
  TileState changeState;
  Direction startDir;
  bool hit, isMoving, canChangeTileState;
  float totalElapsed;
  SpriteProxyNode* shadow;
  SpriteProxyNode* blueShadow;
  std::vector<sf::Vector2f> blueArmShadowPos;
  int blueArmShadowPosIdx;
  float blueShadowTimer; // 1/60 of a second = 1 frame
  bool isSwiping, isFinished;
};
