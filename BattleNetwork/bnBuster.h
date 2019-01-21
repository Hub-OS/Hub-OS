#pragma once
#include "bnSpell.h"
#include "bnAnimate.h"

class Buster : public Spell {
public:
  Buster(Field* _field, Team _team, bool _charged);
  virtual ~Buster(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Entity* _entity);
  virtual vector<Drawable*> GetMiscComponents();
private:
  int damage;
  float cooldown;
  float damageCooldown;
  Animate animator;
  FrameList animation;
};