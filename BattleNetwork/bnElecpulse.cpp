#include <random>
#include <time.h>

#include "bnElecpulse.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Elecpulse::Elecpulse(Field* _field, Team _team, int _damage) : animation(this) {
  this->SetLayer(0);
  SetPassthrough(true);
  this->SetElement(Element::ELEC);
  setScale(2.f, 2.f);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  progress = 0.0f;

  damage = _damage;
  
  setTexture(*TEXTURES.GetTexture(TextureType::SPELL_ELEC_PULSE));

  animation.Setup("resources/spells/elecpulse.animation");
  animation.Reload();

  auto onFinish = [this]() { this->Delete(); };
  animation.SetAnimation("PULSE", onFinish);
}

Elecpulse::~Elecpulse(void) {
}

void Elecpulse::Update(float _elapsed) {
  tile->AffectEntities(this);

  setPosition(tile->getPosition()-sf::Vector2f(-80.0f, 40.0f));
  animation.Update(_elapsed);

  Entity::Update(_elapsed);
}

bool Elecpulse::Move(Direction _direction) {
  return false;
}

void Elecpulse::Attack(Character* _entity) {
  Hit::Properties props;
  props.element = this->GetElement();
  props.flags = Hit::recoil | Hit::stun;
  props.secs = 3;
  props.damage = damage;

   _entity->Hit(props);
   _entity->SlideToTile(true);
   _entity->Move(Direction::LEFT);
}

