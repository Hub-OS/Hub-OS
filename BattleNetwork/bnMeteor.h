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
  Meteor(Team _team, Battle::Tile* target, int damage, float _duration);
  ~Meteor();

  void OnUpdate(double _elapsed) override;
  bool Move(Direction _direction) override;
  void Attack(Character* _entity) override;
  void OnSpawn(Battle::Tile& start) override;
  void OnDelete() override;
};