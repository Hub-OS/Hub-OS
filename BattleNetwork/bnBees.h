
#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Hitbox;

class Bees : public Spell {
protected:
  Animation animation;
  double elapsed;
  Entity* target; /**< The current enemy to approach */
  int damage;
  int turnCount, hitCount;
  bool madeContact; /*!< if a bee hits something, it stays on top of it else it moves*/
  SpriteProxyNode* shadow;
  Bees* leader;/*!< which bee to follow*/
  float attackCooldown; 
  std::list<Hitbox*> dropped;
public:
  Bees(Field* _field, Team _team, int damage);
  Bees(const Bees& leader);
  ~Bees();

  bool CanMoveTo(Battle::Tile* tile);

  void OnUpdate(double _elapsed);

  void Attack(Character* _entity);

  void OnDelete();
};
