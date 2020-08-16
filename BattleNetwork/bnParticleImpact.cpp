#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticleImpact.h"
using sf::IntRect;

#define RESOURCE_PATH "resources/spells/artifact_impact_fx.animation"
#define VULCAN_PATH   "resources/spells/artifact_vulcan_impact.animation"

ParticleImpact::ParticleImpact(ParticleImpact::Type type) : randOffset(), Artifact(nullptr)
{
  SetLayer(-10);
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_IMPACT_FX));
  setScale(2.f, 2.f);

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
  case Type::VULCAN:
    animation = Animation(VULCAN_PATH);
    animation.SetAnimation("DEFAULT");
    setTexture(TEXTURES.GetTexture(TextureType::SPELL_VULCAN_IMPACT_FX));
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
  float height = GetHeight();

  if (type == Type::VULCAN) {
    // stay closer to the body
    height = GetHeight() / 2.0f;
  }

  randOffset = sf::Vector2f(float(rand() % 10), float(rand() % static_cast<int>(height)));
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