#include "bnCanonSmoke.h"
#include "bnCanodumb.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

CanonSmoke::CanonSmoke(Field* _field, Team _team) : animationComponent(this)
{
  SetLayer(0);
  field = _field;
  team = _team;

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
  animationComponent.SetAnimation(MOB_CANODUMB_SMOKE, onFinish);
  animationComponent.Update(0);

}

void CanonSmoke::Update(float _elapsed) {
  setPosition(tile->getPosition().x + (tile->GetWidth() / 2.0f) + 35.0f, tile->getPosition().y + (tile->GetHeight() / 2.0f) - 65.0f);
  animationComponent.Update(_elapsed);
  Entity::Update(_elapsed);
}

vector<Drawable*> CanonSmoke::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();

  return drawables;
}

CanonSmoke::~CanonSmoke()
{
}