#pragma once
#include "bnDefenseRule.h"

class DefenseObstacleBody : public DefenseRule {
public:
  DefenseObstacleBody();
  ~DefenseObstacleBody();
  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) override;
};