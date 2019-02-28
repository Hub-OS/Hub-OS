#pragma once
#include "bnSpell.h"
#include "bnTile.h"

class NinjaStar : public Spell {
private:
  sf::Vector2f start;
  double progress;
  double duration;

public:
  NinjaStar(Field* _field, Team _team, float _duration);
  virtual ~NinjaStar(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
}; 