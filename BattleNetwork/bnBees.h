
#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class HitBox;

class Bees : public Spell {
protected:
  Animation animation;
  double elapsed;
  Entity* target; /**< The current enemy to approach */
  int damage;
  int turnCount, hitCount;
  SpriteSceneNode* shadow;
  Bees* leader;/*!< which bee to follow*/
  float attackCooldown; 
  std::list<HitBox*> dropped;
public:
  Bees(Field* _field, Team _team, int damage);
  Bees(Bees& leader);
  ~Bees();

  bool CanMoveTo(Battle::Tile* tile);

  void OnUpdate(float _elapsed);

  void Attack(Character* _entity);

  void OnDelete();
};
