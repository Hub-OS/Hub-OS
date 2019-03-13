#pragma once
#include <functional>
#include "bnDefenseRule.h"

class DefenseAura : public DefenseRule {
public:
  typedef std::function<void(Spell*in, Character*owner)> Callback;
  
private:
  Callback callback;
  
public:
  DefenseAura(Callback callback);
  DefenseAura();
  
  virtual ~DefenseAura();

  virtual const bool Check(Spell* in, Character* owner);
};
