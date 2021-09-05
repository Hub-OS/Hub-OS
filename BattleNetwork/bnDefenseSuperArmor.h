#pragma once
#include "bnDefenseRule.h"

class DefenseSuperArmor : public DefenseRule {
public:
  DefenseSuperArmor();
  virtual ~DefenseSuperArmor();
  virtual Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  virtual void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};