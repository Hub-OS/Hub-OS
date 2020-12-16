#pragma once
#include "bnSpell.h"

/*!\brief This spell highlights the tile for a specified amount of time before spawning the desired attack*/
class DelayedAttack : public Spell {
private:
  double duration;
  Spell* next;

public:
  DelayedAttack(Spell* next, Battle::Tile::Highlight highlightMode, double seconds);

  ~DelayedAttack();

  void OnUpdate(double elapsed) override;

  void Attack(Character* _entity) override;

  void OnDelete() override;
};