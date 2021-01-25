#pragma once
#include "bnSpell.h"

class Wind : public Spell {
public:

  Wind(Team _team);
  ~Wind();

  void OnUpdate(double _elapsed) override;

  bool CanMoveTo(Battle::Tile* next) override;

  /**
   * @brief moves entities
   * @param _entity
   */
  void Attack(Character* _entity) override;

  void OnDelete() override;
};
