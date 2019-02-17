#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnReflectShield.h"
#include "bnDefenseGuard.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/reflect_shield.animation"

ReflectShield::ReflectShield(Character* owner) : Artifact(), Component(owner)
{
  SetLayer(0);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_REFLECT_SHIELD));
  this->setScale(2.f, 2.f);
  shield = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  this->guard = new DefenseGuard();

  auto onEnd = [this]() {
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->guard);
  };

  animation << Animate::On(5, onEnd, true) << [this]() { this->Delete(); this->GetOwner()->FreeComponent(*this); };

  animation.Update(0, this);

  owner->AddDefenseRule(guard);
}

void ReflectShield::Inject(BattleScene& bs) {

}

void ReflectShield::Update(float _elapsed) {
  this->setPosition(this->tile->getPosition());

  animation.Update(_elapsed, this);
  Entity::Update(_elapsed);
}

vector<Drawable*> ReflectShield::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();

  return drawables;
}

ReflectShield::~ReflectShield()
{
  delete this->guard;
}