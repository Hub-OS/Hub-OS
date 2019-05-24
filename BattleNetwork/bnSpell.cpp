#include <random>
#include <time.h>

#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"


Spell::Spell() : animationComponent(this), Entity() {
  SetFloatShoe(true);
  SetLayer(1);
  direction = Direction::NONE;
  markTile = false;
  hitboxProperties.flags = Hit::none;
}

Spell::~Spell() {
}

const bool Spell::IsTileHighlightEnabled() const {
  return markTile;
}

void Spell::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!isSliding) {
    this->setPosition(tile->getPosition());
  }
}

void Spell::EnableTileHighlight(bool enable)
{
  markTile = enable;
}

void Spell::SetHitboxProperties(Hit::Properties props)
{
  hitboxProperties = props;
}

const Hit::Properties Spell::GetHitboxProperties() const
{
  return this->hitboxProperties;
}
