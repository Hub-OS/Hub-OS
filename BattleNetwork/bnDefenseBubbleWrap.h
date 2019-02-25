#pragma once
#include <functional>
#include "bnDefenseRule.h"

class DefenseBubbleWrap : public DefenseRule {
public:
  DefenseBubbleWrap();

  virtual ~DefenseBubbleWrap();

  virtual const bool Check(Spell* in, Character* owner);
};
