#pragma once
#include "bnSpell.h"

class Wind : public Spell {
public:

  Wind(Field* _field, Team _team);
  ~Wind();

  void OnUpdate(float _elapsed);

  bool CanMoveTo(Battle::Tile* next);

  /**
   * @brief Big pushes entities
   * @param _entity
   */
  void Attack(Character* _entity);
private:
};
