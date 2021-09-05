#include "bnTornado.h"
#include "bnBusterHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnHitbox.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Tornado::Tornado(Team _team, int count, int damage) : 
  damage(damage), 
  count(count),
  Spell(_team) {
  SetLayer(-1);

  setTexture(Textures().GetTexture(TextureType::SPELL_TORNADO));
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    if (this->count-- <= 1) {
      Delete();
    }
  };

  auto firstFrame = [this]() {
    auto hitbox = std::make_shared<Hitbox>(GetTeam(), this->damage);
    auto props = GetHitboxProperties();
    hitbox->SetHitboxProperties(props);

    auto onHit = [this](std::shared_ptr<Character> entity) {
      Audio().Play(AudioType::HURT);
    };

    auto onCollision = [this](const std::shared_ptr<Character> entity) {
      auto hitfx = std::make_shared<BusterHit>(BusterHit::Type::CHARGED);
      GetField()->AddEntity(hitfx, entity->GetTile()->GetX(), entity->GetTile()->GetY());
    };

    hitbox->AddCallback(onHit, onCollision);
    GetField()->AddEntity(hitbox, *GetTile());
  };

  animation = Animation("resources/spells/spell_tornado.animation");
  animation << "DEFAULT" << Animator::On(1, firstFrame) << Animator::Mode::Loop << onFinish;
  animation.Update(0, getSprite());

  HighlightTile(Battle::Tile::Highlight::solid);

  auto props = GetHitboxProperties();
  props.flags &= ~Hit::flash;
  props.flags |= Hit::impact;
  props.damage = damage;
  props.element = Element::wind;
  SetHitboxProperties(props);
}

Tornado::~Tornado() {
}

void Tornado::OnUpdate(double _elapsed) {
  Entity::drawOffset.y = -20.0f;

  animation.Update(_elapsed, getSprite());
}

void Tornado::Attack(std::shared_ptr<Character> _entity) {

}

void Tornado::OnDelete()
{
  Remove();
}
