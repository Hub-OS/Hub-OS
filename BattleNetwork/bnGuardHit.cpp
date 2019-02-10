#include "bnGuardHit.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/guard_hit.animation"

GuardHit::GuardHit(Field* _field, Character* hit) : animationComponent(this)
{
  SetLayer(0);
  field = _field;
  team = Team::UNKNOWN;

  float random = hit->getLocalBounds().width / 2.0f;
  random *= rand() % 2 == 0 ? -1.0f : 1.0f;

  w = (float)random;

  h = (float)(std::floor(hit->GetHitHeight()));

  if (h > 0) {
    h = (float)(rand() % (int)h);
  }

  setTexture(*TEXTURES.GetTexture(TextureType::SPELL_GUARD_HIT));
  setScale(2.f, 2.f);

  //Components setup and load
  auto onFinish = [&]() { this->Delete();  };
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
  animationComponent.SetAnimation("DEFAULT", onFinish);
  animationComponent.Update(0);

  AUDIO.Play(AudioType::GUARD_HIT);
}

void GuardHit::Update(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x + w, tile->getPosition().y + tileOffset.y - h);
  animationComponent.Update(_elapsed);
  Entity::Update(_elapsed);
}

vector<Drawable*> GuardHit::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();

  return drawables;
}

GuardHit::~GuardHit()
{
}