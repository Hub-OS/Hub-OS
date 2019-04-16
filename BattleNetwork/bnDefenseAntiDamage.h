#pragma once
#include <functional>
#include "bnDefenseRule.h"

class DefenseAntiDamage : public DefenseRule {
public:
  typedef std::function<void(Spell* in, Character* owner)> Callback;

private:
  Callback callback;

public:
  DefenseAntiDamage(Callback callback);

  virtual ~DefenseAntiDamage();

  virtual const bool Check(Spell* in, Character* owner);
};
