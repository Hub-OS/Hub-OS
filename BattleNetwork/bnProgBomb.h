#pragma once
#include "bnSpell.h"

class ProgBomb : public Spell {
private:
  Battle::Tile * target;
  float posX;
  float posY;
  float velX;
  float velY;
  float arcDuration;
  float arcProgress;
  float cooldown;
  float damageCooldown;
public:
  ProgBomb(Field* _field, Team _team, Battle::Tile* _target, float _duration);
  virtual ~ProgBomb(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Entity* _entity);
  virtual vector<Drawable*> GetMiscComponents();
  void PrepareThrowPath();
};