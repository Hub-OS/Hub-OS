#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class Aura : virtual public Artifact, virtual public Component
{
public:
  enum class Type : int {
    _100,
    _200,
    _1000,
  };

private:
  Animation animation;
  sf::Sprite aura;
  Type type;
public:
  Aura(Type type, Entity* owner);
  ~Aura();

  virtual void Inject(BattleScene&);
  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
  const Type GetAuraType();
  vector<Drawable*> GetMiscComponents();

};

