#pragma once
#include "bnDefenseRule.h"

class DefenseVirusBody : public DefenseRule {
public:
  DefenseVirusBody();
  ~DefenseVirusBody();
  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  const bool CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner) override;
};