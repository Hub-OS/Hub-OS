#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnMobMoveEffect.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/mob_move.animation"

MobMoveEffect::MobMoveEffect(Field* field) : Artifact(field)
{
  SetLayer(-1);
  this->setTexture(TEXTURES.GetTexture(TextureType::MOB_MOVE));
  this->setScale(2.f, 2.f);
  move = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  auto onEnd = [this]() {
    this->Delete();
  };

  animation << onEnd;

  animation.Update(0, this->getSprite());

}

void MobMoveEffect::OnUpdate(float _elapsed) {
  this->setPosition(this->GetTile()->getPosition());

  animation.Update(_elapsed, this->getSprite());
  //Entity::Update(_elapsed);
}

MobMoveEffect::~MobMoveEffect()
{
}