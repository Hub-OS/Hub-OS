#include "bnBusterHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define CHARGED_RESOURCE_PATH "resources/spells/spell_charged_bullet_hit.animation"
#define PEA_RESOURCE_PATH "resources/spells/spell_bullet_hit.animation"

BusterHit::BusterHit(Field* _field, Type type) : Artifact(_field)
{
  SetLayer(0);
  field = _field;

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };
  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);

  if (type == Type::CHARGED) {
    setTexture(*TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT));
    animationComponent->Setup(CHARGED_RESOURCE_PATH);

  }
  else {
    setTexture(*TEXTURES.GetTexture(TextureType::SPELL_BULLET_HIT));
    animationComponent->Setup(PEA_RESOURCE_PATH);

  }

  animationComponent->Reload();
  animationComponent->SetAnimation("HIT", onFinish);
  animationComponent->OnUpdate(0);

  setScale(2.f, 2.f);
}

void BusterHit::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition() + offset);
}

BusterHit::~BusterHit()
{
}

void BusterHit::SetOffset(const sf::Vector2f offset)
{
  this->offset = { offset.x, -offset.y };
}
