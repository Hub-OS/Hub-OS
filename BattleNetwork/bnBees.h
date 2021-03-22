
#pragma once
#include "bnObstacle.h"
#include "bnAnimation.h"
#include "bnDefenseRule.h"

class Hitbox;

class Bees : public Obstacle {
protected:
  int damage{}, hitCount{}, turnCount{};
  bool madeContact{}; /*!< if a bee hits something, it stays on top of it else it moves*/
  float attackCooldown{};
  double elapsed{};
  Animation animation;
  Entity* target{ nullptr }; /**< The current enemy to approach */
  SpriteProxyNode* shadow{ nullptr };
  Bees* leader{ nullptr };/*!< which bee to follow*/
  DefenseRule* absorbDamage;
public:
  Bees(Team _team,int damage);
  Bees(const Bees& leader);
  ~Bees();

  bool CanMoveTo(Battle::Tile* tile);
  void OnUpdate(double _elapsed);
  void Attack(Character* _entity);
  void OnDelete();
};
