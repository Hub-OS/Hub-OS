#pragma once
#include "bnSceneNode.h"
#include "bnComponent.h"
#include "bnField.h"

class DefenseAura;

class Aura : virtual public SceneNode, virtual public Component
{
public:
  enum class Type : int {
    AURA_100,
    AURA_200,
    AURA_1000,
    BARRIER_100,
    BARRIER_200,
    BARRIER_500
  };

private:
  Animation animation;
  SpriteSceneNode* aura;
  Type type;
  DefenseAura* defense;
  double timer;
  int health;
  bool persist;
  
  int lastHP;
  int currHP;
  int startHP;
  mutable Sprite font;

public:
  Aura(Type type, Character* owner);
  ~Aura();

  virtual void Inject(BattleScene&);
  virtual void Update(float _elapsed);
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
  const Type GetAuraType();
  void TakeDamage(int damage);
  const int GetHealth() const;
  void Persist(bool enable);
  const bool IsPersistent() const;
};

