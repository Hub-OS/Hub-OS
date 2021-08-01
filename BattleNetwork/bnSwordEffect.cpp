#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnAnimationComponent.h"
#include "bnTile.h"
#include "bnSwordEffect.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/spell_sword_slashes.animation"

SwordEffect::SwordEffect() : Artifact()
{
  SetLayer(0);
  setTexture(Textures().GetTexture(TextureType::SPELL_SWORD));

  setScale(2.f, 2.f);

  //Components setup and load
  auto animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath(RESOURCE_PATH);
  animation->Reload();

  // Create a callback
  // When animation ends
  // delete this effect
  auto onEnd = [this]() {
      Delete();
  };
  animation->SetAnimation("DEFAULT", onEnd);

  // Use the first rect in the frame list
  animation->SetFrame(0);
}

void SwordEffect::OnUpdate(double _elapsed) {
}

void SwordEffect::OnDelete()
{
  Remove();
}

void SwordEffect::SetAnimation(const std::string & animStr)
{
  auto animation = GetFirstComponent<AnimationComponent>();
  // Create a callback
  // When animation ends
  // delete this effect
  auto onEnd = [this]() {
      Delete();
  };

  animation->SetAnimation(animStr, onEnd);

  // Use the first rect in the frame list
  animation->SetFrame(0);
}

SwordEffect::~SwordEffect()
{
}
