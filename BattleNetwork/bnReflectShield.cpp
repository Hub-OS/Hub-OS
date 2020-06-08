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
  setTexture(TEXTURES.GetTexture(TextureType::SPELL_REFLECT_SHIELD));
  setScale(2.f, 2.f);
  shield = getSprite();
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
    GetOwnerAs<Character>()->RemoveDefenseRule(guard);
  };

  // Add end callback, flag for deletion, and remove the component from the owner
  // This way the owner doesn't container a pointer to an invalid address
  animation << Animator::On(5, onEnd, true) << [this]() { Delete(); GetOwner()->FreeComponentByID(Component::GetID()); };

  animation.Update(0, getSprite());

  // Add the defense rule to the owner
  owner->AddDefenseRule(guard);
}

void ReflectShield::Inject(BattleScene& bs) {

}

void ReflectShield::OnUpdate(float _elapsed) {
  if (!GetTile()) return;

  setPosition(GetTile()->getPosition());

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

    Audio().Play(AudioType::GUARD_HIT);

    Direction direction = Direction::none;

    if (GetOwner()->GetTeam() == Team::red) {
      direction = Direction::right;
    }
    else if (GetOwner()->GetTeam() == Team::blue) {
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