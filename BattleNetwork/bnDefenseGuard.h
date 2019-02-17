#pragma once
#include "bnDefenseRule.h"

class DefenseGuard : public DefenseRule {
public:
  DefenseGuard();

  virtual ~DefenseGuard();

  virtual const bool Check(Spell* in, Character* owner);
};
