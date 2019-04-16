#pragma once

/*
This class is used specifically for checks in the attack step*/

class Spell;
class Character;

typedef int Priority;

class DefenseRule {
private:
  Priority priorityLevel;

public:
  DefenseRule(Priority level);

  const Priority GetPriorityLevel() const;

  virtual ~DefenseRule();

  // Returns true if spell passes through this defense, false if defense prevents it
  virtual const bool Check(Spell* in, Character* owner) = 0;
};