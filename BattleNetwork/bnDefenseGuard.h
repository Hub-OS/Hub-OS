#pragma once
#include <functional>
#include "bnDefenseRule.h"

class DefenseGuard : public DefenseRule {
public:
  typedef std::function<void(Spell* in, Character* owner)> Callback;

private:
  Callback callback;

public:
  DefenseGuard(Callback callback);

  virtual ~DefenseGuard();

  virtual const bool Check(Spell* in, Character* owner);
};
