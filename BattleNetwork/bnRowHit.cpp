#include "bnRowHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

RowHit::RowHit(Team _team, int damage) : 
  damage(damage), 
  Spell(_team) {
  SetLayer(0);

  setTexture(Textures().GetTexture(TextureType::SPELL_CHARGED_BULLET_HIT));
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    Delete();
  };

  auto onFrameTwo = [this]() {
    field->AddEntity(*new RowHit(GetTeam(), RowHit::damage), tile->GetX() + 1, tile->GetY());
  };

  animation = Animation("resources/spells/spell_charged_bullet_hit.animation");
  animation.SetAnimation("HIT");
  animation << Animator::On(3, onFrameTwo, true) << onFinish;
  animation.Update(0, getSprite());
}

RowHit::~RowHit() {
}

void RowHit::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - 20.0f);

  animation.Update(_elapsed, getSprite());

  tile->AffectEntities(this);
}

void RowHit::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != GetTeam()) {
    auto props = Hit::DefaultProperties;
    props.damage = damage;
    _entity->Hit(props);
  }
}

void RowHit::OnDelete()
{
  Remove();
}
