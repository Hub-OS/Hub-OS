#include "bnGuardHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCharacter.h"

#include <cmath>

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/guard_hit.animation"

GuardHit::GuardHit(Field* _field, Character* hit, bool center) : Artifact(_field)
{
  this->center = center;
  SetLayer(0);
  field = _field;
  team = Team::UNKNOWN;

  if (!center) {
    float random = hit->getLocalBounds().width / 2.0f;
    random *= rand() % 2 == 0 ? -1.0f : 1.0f;

    w = (float)random;

    h = (float)(std::floor(hit->GetHeight()));

    if (h > 0) {
      h = (float)(rand() % (int)h);
    }
  }
  else {
    w = 0;
    h = (float)(std::floor(hit->GetHeight()/2.0f));
  }

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_GUARD_HIT));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };

  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("DEFAULT", onFinish);
  animationComponent->OnUpdate(0);

  AUDIO.Play(AudioType::GUARD_HIT);
}

void GuardHit::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x + w, tile->getPosition().y + tileOffset.y - h);
}

GuardHit::~GuardHit()
{
}
