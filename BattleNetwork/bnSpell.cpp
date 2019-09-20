#include <random>
#include <time.h>

#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"


Spell::Spell(Field* field, Team team) : Entity() {
  SetFloatShoe(true);
  SetLayer(1);
  SetTeam(team);
  SetField(field);
  mode = Battle::Tile::Highlight::none;
  hitboxProperties.flags = Hit::none;
}

Spell::~Spell() {
}

void Spell::Update(float _elapsed) {
  Entity::Update(_elapsed);

  this->OnUpdate(_elapsed);
}
const Battle::Tile::Highlight Spell::GetTileHighlightMode() const {
  return mode;
}

void Spell::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!IsSliding()) {
    this->setPosition(tile->getPosition());
  }
}

void Spell::HighlightTile(Battle::Tile::Highlight mode)
{
  this->mode = mode;
}

void Spell::SetHitboxProperties(Hit::Properties props)
{
  hitboxProperties = props;
}

const Hit::Properties Spell::GetHitboxProperties() const
{
  return this->hitboxProperties;
}
