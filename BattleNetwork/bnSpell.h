#pragma once
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

class Spell : public Entity {
public:
  Spell(void);
  virtual ~Spell(void);

  const bool IsTileHighlightEnabled() const;

  virtual void Update(float _elapsed) = 0;
  virtual bool Move(Direction _direction) = 0;
  virtual void Attack(Entity* _entity) = 0;
  virtual vector<Drawable*> GetMiscComponents() = 0;

  void SetDirection(Direction _direction);
  Direction GetDirection() const;

  void EnableTileHighlight(bool enable);

protected:
  bool hit;
  bool markTile;
  int random;
  float progress;
  float hitHeight;
  Texture* texture;
  Direction direction;
  AnimationComponent animationComponent;
};
