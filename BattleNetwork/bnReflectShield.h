#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ReflectShield : virtual public Artifact, virtual public Component
{
private:
  DefenseRule* guard;
  Animation animation;
  sf::Sprite shield;
public:
  ReflectShield(Character* owner);
  ~ReflectShield();

  virtual void Inject(BattleScene&);
  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }
  vector<Drawable*> GetMiscComponents();

};
