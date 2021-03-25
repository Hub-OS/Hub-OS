#include "bnCanonSmoke.h"
#include "bnCanodumb.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

CanonSmoke::CanonSmoke(): Artifact()
{
  SetLayer(0);

  animationComponent = CreateComponent<AnimationComponent>(this);

  setTexture(Textures().GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { Delete();  };
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Load();
  animationComponent->SetAnimation("SMOKE", onFinish);
  animationComponent->OnUpdate(0);

}

void CanonSmoke::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x + 14.0f, tile->getPosition().y + tileOffset.y - 65.0f);
}

void CanonSmoke::OnDelete()
{
  Remove();
}

CanonSmoke::~CanonSmoke()
{
}