#include "bnFireBurn.h"
#include "bnChargedBusterHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

FireBurn::FireBurn(Field* _field, Team _team, Type type, int damage) : damage(damage), Spell(_field, _team) {
  SetLayer(-1);

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_FIREBURN);
  setTexture(*texture);
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    this->Delete();
  };

  animation = Animation("resources/spells/spell_flame.animation");

  switch (type) {
  case Type::_2:
    animation.SetAnimation("FLAME_2");
    break;
  case Type::_3:
    animation.SetAnimation("FLAME_3");
    break;
  default:
    animation.SetAnimation("FLAME_1");
  }

  animation << onFinish;
  animation.Update(0, *this);

  this->HighlightTile(Battle::Tile::Highlight::solid);

  auto props = GetHitboxProperties();
  props.flags &= ~Hit::recoil;
  props.flags |= Hit::breaking | Hit::flinch;
  props.damage = damage;
  props.element = Element::FIRE;
  this->SetHitboxProperties(props);
}

FireBurn::~FireBurn() {
}

void FireBurn::OnUpdate(float _elapsed) {
  auto height = 30.0f; // above the tile
  auto xoffset = 30.0f; // the flames come out a little from the origin
  setPosition(tile->getPosition().x + xoffset, tile->getPosition().y - height);

  animation.Update(_elapsed, *this);

  // crack the tile it is on
  GetTile()->SetState(TileState::CRACKED);

  tile->AffectEntities(this);
}

bool FireBurn::Move(Direction _direction) {
  return false;
}

void FireBurn::Attack(Character* _entity) {
  if (_entity->Hit(this->GetHitboxProperties())) {
    AUDIO.Play(AudioType::HURT);
  }
}
