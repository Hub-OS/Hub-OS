
#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Bees : public Spell {
protected:
  Animation animation;
  double elapsed;
  Entity* target; /**< The current enemy to approach */
  int damage;
  int turnCount;
  SpriteSceneNode* shadow;

public:
  Bees(Field* _field, Team _team, int damage);
  virtual ~Bees();


  virtual bool CanMoveTo(Battle::Tile* tile);

  virtual void OnUpdate(float _elapsed);

  virtual void Attack(Character* _entity);
};
