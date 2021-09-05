#include <random>
#include <time.h>

#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"


Spell::Spell(Team team) : 
  mode(Battle::Tile::Highlight::none), 
  hitboxProperties(Hit::DefaultProperties), 
  Entity() {
  SetFloatShoe(true);
  SetLayer(1);
  SetTeam(team);
}

Spell::~Spell() {
}

void Spell::Update(double _elapsed) {
  if (IsTimeFrozen()) return;

  Entity::Update(_elapsed);

  OnUpdate(_elapsed);

  setPosition(getPosition().x, getPosition().y - GetHeight());
}
const Battle::Tile::Highlight Spell::GetTileHighlightMode() const {
  return mode;
}

void Spell::AdoptTile(Battle::Tile * tile)
{
  if (!IsMoving()) {
    setPosition(tile->getPosition() + Entity::drawOffset);
  }

  tile->AddEntity(shared_from_this());
}

void Spell::HighlightTile(Battle::Tile::Highlight mode)
{
  Spell::mode = mode;
}

void Spell::SetHitboxProperties(Hit::Properties props)
{
  hitboxProperties = props;
}

const Hit::Properties Spell::GetHitboxProperties() const
{
  return hitboxProperties;
}


