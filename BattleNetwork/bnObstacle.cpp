#include "bnObstacle.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"

Obstacle::Obstacle(Field* _field, Team _team) {
  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  this->field = _field;
  this->team = _team;

  SetFloatShoe(true);
  SetLayer(1);

  direction = Direction::NONE;
  markTile = false;
  hitboxProperties.flags = Hit::none;
}

Obstacle::~Obstacle() {

}

bool Obstacle::CanMoveTo(Battle::Tile * next)
{
  return (Entity::CanMoveTo(next));
}

void Obstacle::AdoptTile(Battle::Tile * tile)
{
  this->Spell::AdoptTile(tile); // favor spell grouping
}

void Obstacle::Update(float _elapsed) {
  Character::Update(_elapsed);
}

void Obstacle::SetAnimation(std::string animation)
{
  this->animationComponent.SetAnimation(animation);
}
