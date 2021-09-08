#include "bnGuardHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCharacter.h"

#include <cmath>

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/guard_hit.animation"

GuardHit::GuardHit(std::shared_ptr<Character> hit, bool center) 
  : 
  w(0), 
  h(0), 
  center(true), 
  Artifact()
{
  SetLayer(0);
  team = Team::unknown;

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

  Entity::drawOffset = { w, -h };

  setTexture(Textures().GetTexture(TextureType::SPELL_GUARD_HIT));
  setScale(2.f, 2.f);

  Audio().Play(AudioType::GUARD_HIT);
}

void GuardHit::Init() {
  Artifact::Init();

  //Components setup and load
  auto onFinish = [&]() { Delete();  };

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("DEFAULT", onFinish);
  animationComponent->OnUpdate(0);
}

void GuardHit::OnUpdate(double _elapsed) {

}

void GuardHit::OnDelete()
{
  Remove();
}

GuardHit::~GuardHit()
{
}
