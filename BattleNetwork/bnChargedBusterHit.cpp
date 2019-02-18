#include "bnChargedBusterHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/guard_hit.animation"

ChargedBusterHit::ChargedBusterHit(Field* _field, Character* hit) : animationComponent(this)
{
  SetLayer(0);
  field = _field;
  team = Team::UNKNOWN;

  setTexture(*TEXTURES.GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
  animationComponent.SetAnimation("DEFAULT", onFinish);
  animationComponent.Update(0);

  AUDIO.Play(AudioType::GUARD_HIT);
}

void ChargedBusterHit::Update(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y);
  animationComponent.Update(_elapsed);
  Entity::Update(_elapsed);
}

ChargedBusterHit::~ChargedBusterHit()
{
}