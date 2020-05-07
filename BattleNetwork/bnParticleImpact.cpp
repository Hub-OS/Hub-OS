#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticleImpact.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/artifact_impact_fx.animation"

ParticleImpact::ParticleImpact(ParticleImpact::Type type) : Artifact(nullptr)
{
  SetLayer(0);
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_IMPACT_FX));
  setScale(2.f, 2.f);
  fx = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch(type) {
  case Type::GREEN:
    animation.SetAnimation("GREEN");
    break;
  case Type::YELLOW:
    animation.SetAnimation("YELLOW");
    break;
  case Type::BLUE:
    animation.SetAnimation("BLUE");
    break;
  case Type::THIN:
    animation.SetAnimation("THIN");
    break;
  case Type::FIRE:
    animation.SetAnimation("FIRE");
    break;
  default:
    animation.SetAnimation("GREEN");
  }

  auto onEnd = [this]() {
    Delete();
  };

  animation << onEnd;

  animation.Update(0, getSprite());

}

bool ParticleImpact::Move(Direction _direction)
{
  return false;
}

void ParticleImpact::OnSpawn(Battle::Tile& tile) {
  randOffset = sf::Vector2f(float(rand() % 10), float(rand() % 10));
  randOffset.x *= rand() % 2 ? -1 : 1;
  randOffset.y = randOffset.y - GetHeight();
}

void ParticleImpact::OnUpdate(float _elapsed) {
  animation.Update(_elapsed, getSprite());
  Entity::Update(_elapsed);

  setPosition(GetTile()->getPosition() + tileOffset + randOffset);
}

void ParticleImpact::OnDelete()
{
  Remove();
}

ParticleImpact::~ParticleImpact()
{
}