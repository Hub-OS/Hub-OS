#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnReflectShield.h"
#include "bnDefenseGuard.h"

using sf::IntRect;

<<<<<<< HEAD
#define RESOURCE_PATH "resources/spells/reflect_shield.animation"
=======
const std::string RESOURCE_PATH = "resources/spells/reflect_shield.animation";
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

ReflectShield::ReflectShield(Character* owner) : Artifact(), Component(owner)
{
  SetLayer(0);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_REFLECT_SHIELD));
  this->setScale(2.f, 2.f);
  shield = (sf::Sprite)*this;
  activated = false;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

<<<<<<< HEAD
=======
  // Specify the guard callback when it fails
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  DefenseGuard::Callback callback = [this](Spell* in, Character* owner) {
    this->DoReflect(in, owner);
  };

<<<<<<< HEAD
  this->guard = new DefenseGuard(callback);

=======
  // Build a basic guard rule
  this->guard = new DefenseGuard(callback);

  // When the animtion ends, remove the defense rule
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  auto onEnd = [this]() {
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->guard);
  };

<<<<<<< HEAD
=======
  // Add end callback, flag for deletion, and remove the component from the owner
  // This way the owner doesn't container a pointer to an invalid address
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  animation << Animate::On(5, onEnd, true) << [this]() { this->Delete(); this->GetOwner()->FreeComponentByID(this->Component::GetID()); };

  animation.Update(0, *this);

<<<<<<< HEAD
=======
  // Add the defense rule to the owner
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  owner->AddDefenseRule(guard);
}

void ReflectShield::Inject(BattleScene& bs) {

}

void ReflectShield::Update(float _elapsed) {
  this->setPosition(this->tile->getPosition());

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
}

void ReflectShield::DoReflect(Spell* in, Character* owner)
{
  if (!this->activated) {
    Direction direction = Direction::NONE;

<<<<<<< HEAD
    if (in->GetDirection() == Direction::LEFT) {
      direction = Direction::RIGHT;
    }
    else if (in->GetDirection() == Direction::RIGHT) {
=======
    if (GetOwner()->GetTeam() == Team::RED) {
      direction = Direction::RIGHT;
    }
    else if (GetOwner()->GetTeam() == Team::BLUE) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      direction = Direction::LEFT;
    }

    Field* field = owner->GetField();

    Spell* rowhit = new RowHit(field, owner->GetTeam(), 60);
    rowhit->SetDirection(direction);

    field->AddEntity(*rowhit, owner->GetTile()->GetX() + 1, owner->GetTile()->GetY());

<<<<<<< HEAD
=======
    // Only emit a row hit once
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    this->activated = true;
  }
}

ReflectShield::~ReflectShield()
{
  delete this->guard;
}