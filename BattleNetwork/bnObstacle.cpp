#include "bnObstacle.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"

Obstacle::Obstacle(Field* _field, Team _team) : Spell(_field, _team), Character()  {
  this->field = _field;
  this->team = _team;

  SetFloatShoe(true);
  SetLayer(1);
  hitboxProperties.flags = Hit::none;
}

Obstacle::~Obstacle() {

}

void Obstacle::Update(float _elapsed)
{
  Spell::Update(_elapsed);
  Character::Update(_elapsed);
}

bool Obstacle::CanMoveTo(Battle::Tile * next)
{
  return (Entity::CanMoveTo(next));
}

void Obstacle::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!IsSliding()) {
    this->setPosition(tile->getPosition());
  }
}

