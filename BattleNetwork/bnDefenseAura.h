#pragma once
#include <functional>
#include "bnDefenseRule.h"

class DefenseAura : public DefenseRule {
public:
  DefenseAura();

  virtual ~DefenseAura();

  virtual const bool Check(Spell* in, Character* owner);
};
