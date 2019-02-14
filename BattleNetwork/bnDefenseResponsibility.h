#pragma once

/*
This class is used specifically for checks in the attack step*/

class Spell;

class DefenseResponsibility {
private:
  DefenseResponsibility* next;

protected:
  const bool Next(Spell *in);

public:
  DefenseResponsibility();

  virtual ~DefenseResponsibility();

  void Add(DefenseResponsibility* in);

  virtual const bool Check(Spell* in) = 0;
};