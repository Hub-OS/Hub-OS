#pragma once
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

class Spell : public virtual Entity {
public:
  Spell();
  virtual ~Spell();

  const bool IsTileHighlightEnabled() const;

  virtual void Update(float _elapsed) = 0;
  virtual void Attack(Character* _entity) = 0;
  virtual void AdoptTile(Battle::Tile* tile);

  void EnableTileHighlight(bool enable);

  void SetHitboxProperties(Hit::Properties props);
  const Hit::Properties GetHitboxProperties() const;

protected:
  bool hit;
  bool markTile;
  float progress;
  float hitHeight;
  Texture* texture;
  AnimationComponent animationComponent;
  Hit::Properties hitboxProperties;
};
