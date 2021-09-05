#pragma once
#include "bnDefenseRule.h"

class DefenseObstacleBody : public DefenseRule {
public:
  DefenseObstacleBody();
  ~DefenseObstacleBody();
  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};