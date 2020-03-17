#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnReflectShield.h"
#include "bnDefenseGuard.h"

using sf::IntRect;

const std::string RESOURCE_PATH = "resources/spells/reflect_shield.animation";

ReflectShield::ReflectShield(Character* owner, int damage) : damage(damage), Artifact(nullptr), Component(owner)
{
  SetLayer(0);
  this->setTexture(TEXTURES.GetTexture(TextureType::SPELL_REFLECT_SHIELD));
  this->setScale(2.f, 2.f);
  shield = (sf::Sprite)*this;
  activated = false;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  // Specify the guard callback when it fails
  DefenseGuard::Callback callback = [this](Spell* in, Character* owner) {
    this->DoReflect(in, owner);
  };

  // Build a basic guard rule
  this->guard = new DefenseGuard(callback);

  // When the animtion ends, remove the defense rule
  auto onEnd = [this]() {
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->guard);
  };

  // Add end callback, flag for deletion, and remove the component from the owner
  // This way the owner doesn't container a pointer to an invalid address
  animation << Animator::On(5, onEnd, true) << [this]() { this->Delete(); this->GetOwner()->FreeComponentByID(this->Component::GetID()); };

  animation.Update(0, *this);

  // Add the defense rule to the owner
  owner->AddDefenseRule(guard);
}

void ReflectShield::Inject(BattleScene& bs) {

}

void ReflectShield::OnUpdate(float _elapsed) {
  if (!this->GetTile()) return;

  this->setPosition(this->GetTile()->getPosition());

  animation.Update(_elapsed, *this);
}

void ReflectShield::DoReflect(Spell* in, Character* owner)
{
  if (!this->activated) {

    AUDIO.Play(AudioType::GUARD_HIT);

    Direction direction = Direction::NONE;

    if (GetOwner()->GetTeam() == Team::RED) {
      direction = Direction::RIGHT;
    }
    else if (GetOwner()->GetTeam() == Team::BLUE) {
      direction = Direction::LEFT;
    }

    Field* field = owner->GetField();

    Spell* rowhit = new RowHit(field, owner->GetTeam(), damage);
    rowhit->SetDirection(direction);

    field->AddEntity(*rowhit, owner->GetTile()->GetX() + 1, owner->GetTile()->GetY());

    // Only emit a row hit once
    this->activated = true;
  }
}

ReflectShield::~ReflectShield()
{
  delete this->guard;
}