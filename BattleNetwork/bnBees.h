
#pragma once
#include "bnObstacle.h"
#include "bnAnimation.h"
#include "bnDefenseAura.h"
#include "bnParticleImpact.h"
class Hitbox;

namespace {
  constexpr auto beeCallback = [](Spell& spell, Character& in) -> void {
    if (spell.GetHitboxProperties().element == Element::fire) { 
      in.Delete(); 
      ParticleImpact* fx = new ParticleImpact(ParticleImpact::Type::volcano);
      fx->SetHeight(in.GetHeight());
      fx->SetOffset(in.GetTileOffset());
      in.GetField()->AddEntity(*fx, *in.GetTile());
    }
  };
}

class Bees : public Obstacle {
  class BeeDefenseRule : public DefenseAura {


  public:
    BeeDefenseRule() : DefenseAura(::beeCallback)
    {
      // silence is golden
    }
  };
protected:
  int damage{}, hitCount{}, turnCount{};
  bool madeContact{}; /*!< if a bee hits something, it stays on top of it else it moves*/
  float attackCooldown{};
  double elapsed{};
  Animation animation;
  Entity* target{ nullptr }; /**< The current enemy to approach */
  SpriteProxyNode* shadow{ nullptr };
  Bees* leader{ nullptr };/*!< which bee to follow*/
  DefenseAura* absorbDamage;
public:
  Bees(Team _team,int damage);
  Bees(const Bees& leader);
  ~Bees();

  bool CanMoveTo(Battle::Tile* tile);
  void OnUpdate(double _elapsed);
  void Attack(Character* _entity);
  void OnDelete();
};
