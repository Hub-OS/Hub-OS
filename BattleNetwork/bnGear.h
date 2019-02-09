#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/*
    Gear slides left and right slowly
    Gear cannot collide with other obstacles
    If it's coming up on one, it reverses direction
    Gears are impervious to damage
*/

class Gear : public Obstacle {
public:
  Gear(Field * _field, Team _team, Direction startDir);
  virtual ~Gear(void);

  virtual void Update(float _elapsed);
  virtual const bool Hit(int damage, Hit::Properties props = Character::DefaultHitProperties);
  virtual void OnDelete();
  virtual void SetAnimation(std::string animation);
  virtual bool CanMoveTo(Battle::Tile * next);
  virtual void Attack(Character* e);
  vector<Drawable*> GetMiscComponents() { return vector<Drawable*>(); }

protected:
  Texture* texture;
  AnimationComponent animation;
};
