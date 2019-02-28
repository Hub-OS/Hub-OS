#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

/* 
   Adds and removes defense rule for antidamage checks.
   Spawns ninja stars in retaliation
*/

class DefenseRule;

class NinjaAntiDamage : public Component {
private:
  DefenseRule* defense;

public:
  NinjaAntiDamage(Entity* owner);
  ~NinjaAntiDamage();

  virtual void Update(float _elapsed);
  virtual void Inject(BattleScene&);
};
