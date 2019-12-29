#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class Vulcan : public Spell {
public:

  Vulcan(Field* _field, Team _team, int _damage);
  ~Vulcan();

  void OnUpdate(float _elapsed);

  bool CanMoveTo(Battle::Tile* next);

  /**
   * @brief Deal impact damage
   * @param _entity
   */
  void Attack(Character* _entity);
private:
  int damage;
  float cooldown;
  float random; // scatter offset
  float hitHeight;
};