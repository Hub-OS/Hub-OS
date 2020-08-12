#pragma once
#include "bnSpell.h"
class AnimationComponent;

class Lightning : public Spell {
  AnimationComponent* anim;

public:
  Lightning(Field* field, Team team);
  ~Lightning();

  // Inherited via Spell
  void OnSpawn(Battle::Tile& start) override;
  void OnUpdate(float _elapsed) override;
  void Attack(Character * _entity) override;
  void OnDelete() override;
};