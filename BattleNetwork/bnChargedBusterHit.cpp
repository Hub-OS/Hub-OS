#include "bnChargedBusterHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/spell_charged_bullet_hit.animation"

ChargedBusterHit::ChargedBusterHit(Field* _field) : Artifact(_field)
{
  SetLayer(0);
  field = _field;
  team = Team::UNKNOWN;

  setTexture(*TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };
  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("HIT", onFinish);
  animationComponent->OnUpdate(0);
}

void ChargedBusterHit::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x, GetTile()->getPosition().y);
}

ChargedBusterHit::~ChargedBusterHit()
{
}