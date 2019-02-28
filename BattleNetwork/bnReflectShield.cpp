#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
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

  DefenseGuard::Callback callback = [](Spell* in, Character* owner) {
    Direction direction = Direction::NONE;

    if (in->GetDirection() == Direction::LEFT) {
      direction = Direction::RIGHT;
    }
    else if (in->GetDirection() == Direction::RIGHT) {
      direction = Direction::LEFT;
    }

    Field* field = owner->GetField();

    Spell* rowhit = new RowHit(field, owner->GetTeam(), 60);
    rowhit->SetDirection(direction);

    field->AddEntity(*rowhit, owner->GetTile()->GetX()+1, owner->GetTile()->GetY());
  };

  this->guard = new DefenseGuard(callback);

  auto onEnd = [this]() {
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->guard);
  };

  animation << Animate::On(5, onEnd, true) << [this]() { this->Delete(); this->GetOwner()->FreeComponentByID(this->Component::GetID()); };

  animation.Update(0, *this);

  owner->AddDefenseRule(guard);
}

void ReflectShield::Inject(BattleScene& bs) {

}

void ReflectShield::Update(float _elapsed) {
  this->setPosition(this->tile->getPosition());

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
}

ReflectShield::~ReflectShield()
{
  delete this->guard;
}