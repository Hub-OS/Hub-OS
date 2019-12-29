#pragma once
#include "bnSpell.h"

class MiniBomb : public Spell {
private:
  int random;
  sf::Vector2f start;
  float arcDuration;
  float arcProgress;
  float cooldown;
  float damageCooldown;

public:
  MiniBomb(Field* _field, Team _team, sf::Vector2f startPos, float _duration, int damage);
  virtual ~MiniBomb(void);

  virtual void OnUpdate(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};