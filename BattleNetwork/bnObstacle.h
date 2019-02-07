#pragma once

#pragma once
#include "bnCharacter.h"
#include "bnSpell.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/*
    Obstacles are characters in the sense they can be spawned, but they generally deal damage
    to entities occupying the same tile.
*/

class Obstacle : public Character, public  Spell {
public:
  Obstacle(Field* _field, Team _team);
  virtual ~Obstacle(void);

  virtual void Update(float _elapsed);
  virtual const bool Hit(int damage, Hit::Properties props = Entity::DefaultHitProperties) = 0;
  virtual void SetAnimation(std::string animation);
  virtual bool CanMoveTo(Battle::Tile * next);
  virtual void Attack(Entity* _entity) = 0;

  vector<Drawable*> GetMiscComponents() { return vector<Drawable*>(); }
  double timer;

protected:
  sf::Shader* whiteout;
};