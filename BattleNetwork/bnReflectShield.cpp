#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnRowHit.h"
#include "bnReflectShield.h"
#include "bnDefenseGuard.h"

using sf::IntRect;

const std::string RESOURCE_PATH = "resources/spells/reflect_shield.animation";

ReflectShield::ReflectShield(std::shared_ptr<Character> owner, int damage, Type type) : 
  damage(damage), 
  owner(owner),
  type(type),
  duration(seconds_cast<float>(frames(63))),
  Artifact()
{
  setTexture(Textures().GetTexture(TextureType::SPELL_REFLECT_SHIELD));
  SetLayer(-1); // stay above characters
  setScale(sf::Vector2f(2.f, 2.f));
  activated = false;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch (type) {
  case Type::red:
    animation.SetAnimation("RED");
    break;
  case Type::yellow:
  default:
    animation.SetAnimation("YELLOW");
  }

  animation.Update(0, getSprite());

  // Specify the guard callback when it fails
  DefenseGuard::Callback callback = [this](auto in, auto owner) {
    DoReflect(*in, *owner);
  };

  // Build a basic guard rule
  guard = std::make_shared<DefenseGuard>(callback);


  // Add the defense rule to the owner
  owner->AddDefenseRule(guard);

  if (owner->GetTeam() == Team::blue) {
    setScale(-getScale().x, getScale().y);
  }
}

void ReflectShield::OnUpdate(double _elapsed) {

  duration -= _elapsed;

  animation.Update(_elapsed, getSprite());

  if (duration <= 0 && state == State::defending) {
    state = State::dissolving;

    animation << "DEFAULT";

    // When the animtion ends, remove the defense rule
    auto onEnd = [this]() {
      this->owner->RemoveDefenseRule(guard);
    };

    // Add end callback, flag for deletion
    animation << Animator::On(2, onEnd, true) << [this]() { Delete(); };

    animation.Update(0, getSprite());
  }
}

void ReflectShield::OnDelete()
{
  Remove();
}

void ReflectShield::DoReflect(Spell& in, Character& owner)
{
  if (!activated) {

    Audio().Play(AudioType::GUARD_HIT);

    Direction direction = Direction::none;

    if (owner.GetTeam() == Team::red) {
      direction = Direction::right;
    }
    else if (owner.GetTeam() == Team::blue) {
      direction = Direction::left;
    }

    Field* field = owner.GetField();

    auto rowhit = std::make_shared<RowHit>(owner.GetTeam(), damage);
    rowhit->SetDirection(direction);

    field->AddEntity(rowhit, owner.GetTile()->GetX() + 1, owner.GetTile()->GetY());

    // Only emit a row hit once
    activated = true;
  }
}

void ReflectShield::SetDuration(const frame_time_t& frames)
{
  duration = seconds_cast<float>(frames);
}

ReflectShield::~ReflectShield()
{
  
}