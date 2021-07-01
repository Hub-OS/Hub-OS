#include "bnFireBurn.h"
#include "bnParticleImpact.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

FireBurn::FireBurn(Team _team, Type type, int damage) : damage(damage), Spell(_team) {
  SetLayer(-1);

  setTexture(Textures().GetTexture(TextureType::SPELL_FIREBURN));
  setScale(2.f, 2.f);

  if (_team == Team::blue) {
    setScale(-2.f, 2.f);
  }

  //When the animation ends, delete this
  auto onFinish = [this]() {
    Delete();
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
  animation.Update(0, getSprite());

  HighlightTile(Battle::Tile::Highlight::solid);

  auto props = GetHitboxProperties();
  props.flags &= ~Hit::flinch;
  props.flags |= Hit::breaking | Hit::flash;
  props.damage = damage;
  props.element = Element::fire;
  SetHitboxProperties(props);
}

FireBurn::~FireBurn() {
}

void FireBurn::OnUpdate(double _elapsed) {
  auto xoffset = 38.0f; // the flames come out a little from the origin
  setPosition(tile->getPosition().x + xoffset, tile->getPosition().y);

  animation.Update(_elapsed, getSprite());

  tile->AffectEntities(this);
}

void FireBurn::OnSpawn(Battle::Tile& start)
{
  // crack the tile it is on
  if (crackTiles) {
    auto state = start.GetState();

    if (state == TileState::cracked) {
      start.SetState(TileState::broken);
    }
    else if (state != TileState::broken && state != TileState::empty) {
      start.SetState(TileState::cracked);
    }
  }
}

void FireBurn::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    // X hit effect when hit by fire
    auto fx = new ParticleImpact(ParticleImpact::Type::volcano);
    fx->SetOffset({ _entity->GetTileOffset().x, -_entity->GetHeight() });
    GetField()->AddEntity(*fx, *GetTile());

    Audio().Play(AudioType::HURT);
  }
}

void FireBurn::OnDelete()
{
  Remove();
}

void FireBurn::CrackTiles(bool state)
{
  crackTiles = state;
}
