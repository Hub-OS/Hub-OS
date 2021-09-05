#pragma once
#include "bnSpell.h"

/*!\brief This spell highlights the tile for a specified amount of time before spawning the desired attack*/
class DelayedAttack : public Spell {
private:
  double duration;
  std::shared_ptr<Spell> next;

public:
  DelayedAttack(std::shared_ptr<Spell> next, Battle::Tile::Highlight highlightMode, double seconds);

  ~DelayedAttack();

  void OnUpdate(double elapsed) override;

  void Attack(std::shared_ptr<Character> _entity) override;

  void OnDelete() override;
};