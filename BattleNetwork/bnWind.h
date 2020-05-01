#pragma once
#include "bnSpell.h"

class Wind : public Spell {
public:

  Wind(Field* _field, Team _team);
  ~Wind();

  void OnUpdate(float _elapsed) override;

  bool CanMoveTo(Battle::Tile* next) override;

  /**
   * @brief moves entities
   * @param _entity
   */
  void Attack(Character* _entity) override;

  void OnDelete() override;
};
