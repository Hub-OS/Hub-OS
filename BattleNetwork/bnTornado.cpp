#include "bnTornado.h"
#include "bnBusterHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Tornado::Tornado(Field* _field, Team _team, int count, int damage) 
  : damage(damage), 
  count(count),
  Spell(_field, _team) {
  SetLayer(-1);

  setTexture(Textures().GetTexture(TextureType::SPELL_TORNADO));
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    if (this->count-- <= 0) {
      Delete();
    }
  };

  auto firstFrame = [this]() {
    Hitbox* hitbox = new Hitbox(GetField(), GetTeam(), this->damage);
    auto props = GetHitboxProperties();
    hitbox->SetHitboxProperties(props);

    auto onHit = [this](Character* entity) {
      if (entity->Hit(GetHitboxProperties())) {
        Audio().Play(AudioType::HURT);

        Artifact* hitfx = new BusterHit(GetField(), BusterHit::Type::CHARGED);
        GetField()->AddEntity(*hitfx, entity->GetTile()->GetX(), entity->GetTile()->GetY());
      }
    };

    hitbox->AddCallback(onHit);
    GetField()->AddEntity(*hitbox, *GetTile());
  };

  animation = Animation("resources/spells/spell_tornado.animation");
  animation << "DEFAULT" << Animator::On(1, firstFrame) << Animator::Mode::Loop << onFinish;
  animation.Update(0, getSprite());

  HighlightTile(Battle::Tile::Highlight::solid);

  auto props = GetHitboxProperties();
  props.flags &= ~Hit::recoil;
  props.flags |= Hit::impact;
  props.damage = damage;
  props.element = Element::wind;
  SetHitboxProperties(props);
}

Tornado::~Tornado() {
}

void Tornado::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - 20.0f);

  animation.Update(_elapsed, getSprite());
}

bool Tornado::Move(Direction _direction) {
  return false;
}

void Tornado::Attack(Character* _entity) {

}

void Tornado::OnDelete()
{
  Remove();
}
