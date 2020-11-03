#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnReflectShield.h"
#include "bnDefenseGuard.h"

using sf::IntRect;

const std::string RESOURCE_PATH = "resources/spells/reflect_shield.animation";

ReflectShield::ReflectShield(Character* owner, int damage) 
  : damage(damage), 
  owner(owner),
  Artifact(owner->GetField())
{
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_REFLECT_SHIELD));
  SetLayer(-1); // stay above characters
  setScale(sf::Vector2f(2.f, 2.f));

  activated = false;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  // Specify the guard callback when it fails
  DefenseGuard::Callback callback = [this](Spell& in, Character& owner) {
    DoReflect(in, owner);
  };

  // Build a basic guard rule
  guard = new DefenseGuard(callback);

  // When the animtion ends, remove the defense rule
  auto onEnd = [this]() {
    this->owner->RemoveDefenseRule(guard);
  };

  // Add end callback, flag for deletion
  animation << Animator::On(2, onEnd, true) << [this]() { Delete(); };

  animation.Update(0, getSprite());

  // Add the defense rule to the owner
  owner->AddDefenseRule(guard);

  if (owner->GetTeam() == Team::blue) {
    setScale(-getScale().x, getScale().y);
  }
}

void ReflectShield::OnUpdate(float _elapsed) {
  animation.Update(_elapsed, getSprite());
}

void ReflectShield::OnDelete()
{
  Remove();
}

bool ReflectShield::Move(Direction _direction)
{
  return false;
}

void ReflectShield::DoReflect(Spell& in, Character& owner)
{
  if (!activated) {

    AUDIO.Play(AudioType::GUARD_HIT);

    Direction direction = Direction::none;

    if (owner.GetTeam() == Team::red) {
      direction = Direction::right;
    }
    else if (owner.GetTeam() == Team::blue) {
      direction = Direction::left;
    }

    Field* field = owner.GetField();

    Spell* rowhit = new RowHit(field, owner.GetTeam(), damage);
    rowhit->SetDirection(direction);

    field->AddEntity(*rowhit, owner.GetTile()->GetX() + 1, owner.GetTile()->GetY());

    // Only emit a row hit once
    activated = true;
  }
}

ReflectShield::~ReflectShield()
{
  delete guard;
}