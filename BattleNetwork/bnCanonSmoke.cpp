#include "bnCanonSmoke.h"
#include "bnCanodumb.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

CanonSmoke::CanonSmoke(Field* _field) : Artifact(_field)
{
  this->SetLayer(0);

  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);

  setTexture(TEXTURES.GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Load();
  animationComponent->SetAnimation(MOB_CANODUMB_SMOKE, onFinish);
  animationComponent->OnUpdate(0);

}

void CanonSmoke::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 65.0f);
}

CanonSmoke::~CanonSmoke()
{
}