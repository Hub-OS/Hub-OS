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

  AlphaArm(Team _team, AlphaArm::Type type);
  ~AlphaArm();

  void OnUpdate(double _elapsed) override;
 
  void OnDelete() override;

  bool CanMoveTo(Battle::Tile * next) override;

  void Attack(Character* e) override;

  const float GetHeight() const;

  const bool IsSwiping() const;

  void SyncElapsedTime(const double elapsedTime);

  void LeftArmChangesTileState();
  void RightArmChangesTileTeam();

  const bool GetIsFinished() const;

private:
  TileState changeState;
  Direction startDir;
  bool hit, isMoving, canChangeTileState;
  double totalElapsed;
  SpriteProxyNode* shadow;
  SpriteProxyNode* blueShadow;
  std::vector<sf::Vector2f> blueArmShadowPos;
  int blueArmShadowPosIdx;
  double blueShadowTimer; // 1/60 of a second = 1 frame
  bool isSwiping, isFinished;
};
