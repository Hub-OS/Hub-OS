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
  virtual ~Meteor();

  virtual void OnUpdate(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};