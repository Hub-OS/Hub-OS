#pragma once
#include "bnSpell.h"

/*!\brief This spell highlights the tile for a specified amount of time before spawning the desired attack*/
class DelayedAttack : public Spell {
private:
  double duration;
  std::shared_ptr<Entity> next;

public:
  DelayedAttack(std::shared_ptr<Entity> next, Battle::TileHighlight highlightMode, double seconds);

  ~DelayedAttack();

  void OnUpdate(double elapsed) override;

  void Attack(std::shared_ptr<Entity> _entity) override;

  void OnDelete() override;
};