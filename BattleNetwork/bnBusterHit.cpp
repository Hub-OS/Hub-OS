#include "bnBusterHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define CHARGED_RESOURCE_PATH "resources/spells/spell_charged_bullet_hit.animation"
#define PEA_RESOURCE_PATH "resources/spells/spell_bullet_hit.animation"

BusterHit::BusterHit(Type type) : Artifact(), type(type)
{
  SetLayer(0);
  setScale(2.f, 2.f);
}

void BusterHit::Init()
{
  Artifact::Init();

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());

  if (type == Type::CHARGED) {
    setTexture(Textures().LoadFromFile(TexturePaths::SPELL_CHARGED_BULLET_HIT));
    animationComponent->SetPath(CHARGED_RESOURCE_PATH);
  }
  else {
    setTexture(Textures().LoadFromFile(TexturePaths::SPELL_BULLET_HIT));
    animationComponent->SetPath(PEA_RESOURCE_PATH);
  }

  auto onFinish = [&]() { Delete(); };

  animationComponent->Reload();
  animationComponent->SetAnimation("HIT", onFinish);
  animationComponent->OnUpdate(0);
}

void BusterHit::OnUpdate(double _elapsed) {
}

void BusterHit::OnDelete()
{
  Remove();
}

BusterHit::~BusterHit()
{
}

void BusterHit::SetOffset(const sf::Vector2f offset)
{
  this->offset = { offset.x, offset.y };
  Entity::drawOffset = this->offset;
}
