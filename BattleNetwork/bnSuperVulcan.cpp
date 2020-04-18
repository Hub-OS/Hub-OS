#include "bnSuperVulcan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

SuperVulcan::SuperVulcan(Field* _field, Team _team, int damage) : damage(damage), Spell(_field, _team) {
  SetLayer(1);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_SUPER_VULCAN));
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    this->Delete();
  };

  animation = Animation("resources/spells/spell_super_vulcan.animation");
  animation.Load();
  animation.SetAnimation("DEFAULT");
  animation << onFinish;
  animation.Update(0, this->getSprite());

  AUDIO.Play(AudioType::GUN, AudioPriority::HIGHEST);

  auto props = GetHitboxProperties();
  props.damage = damage;
  this->SetHitboxProperties(props);
}

SuperVulcan::~SuperVulcan() {
}

void SuperVulcan::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y);

  animation.Update(_elapsed, this->getSprite());

  tile->AffectEntities(this);
}

bool SuperVulcan::Move(Direction _direction) {
  return false;
}

void SuperVulcan::Attack(Character* _entity) {
  if (_entity->Hit(this->GetHitboxProperties())) {
    AUDIO.Play(AudioType::HURT);
  }
}
