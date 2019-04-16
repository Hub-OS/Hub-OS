#pragma once
#include "bnSpell.h"

class ProgBomb : public Spell {
private:
  int random;
  sf::Vector2f start;
  float arcDuration;
  float arcProgress;
  float cooldown;
  float damageCooldown;
public:
  ProgBomb(Field* _field, Team _team, sf::Vector2f startPos, float _duration);
  virtual ~ProgBomb(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};