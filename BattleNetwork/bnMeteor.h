#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

namespace Battle {
  class Tile;
};

class Meteor : public Spell {
private:
  sf::Vector2f start;
  Battle::Tile* target;

  double progress;
  double duration;

public:
  Meteor(Field* _field, Team _team, Battle::Tile* target, int damage, float _duration);
  ~Meteor();

  void OnUpdate(float _elapsed) override;
  bool Move(Direction _direction) override;
  void Attack(Character* _entity) override;
  void OnDelete() override;
};