#pragma once
#include "bnDefenseRule.h"

class DefenseVirusBody : public DefenseRule {
public:
  DefenseVirusBody();
  virtual ~DefenseVirusBody();
  virtual Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  virtual void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Spell> in, std::shared_ptr<Character> owner) override;
};