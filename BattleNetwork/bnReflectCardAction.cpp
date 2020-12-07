#include "bnReflectCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define FRAME1 { 1, 1.3f }

#define FRAMES FRAME1


<<<<<<< HEAD
ReflectCardAction::ReflectCardAction(Character * owner, int damage, ReflectShield::Type type) :
  CardAction(*owner, "PLAYER_IDLE"),
  type(type) 
{
=======
ReflectCardAction::ReflectCardAction(Character& owner, int damage) : CardAction(owner, "PLAYER_IDLE") {
>>>>>>> menu-refactor
  ReflectCardAction::damage = damage;

  // default override anims
  OverrideAnimationFrames({ FRAMES });
}

ReflectCardAction::~ReflectCardAction()
{
}

<<<<<<< HEAD
void ReflectCardAction::Execute() {
  auto user = GetOwner();

  // Create a new reflect shield component. This handles the logic for shields.
  ReflectShield* reflect = new ReflectShield(user, damage, type);
  reflect->SetDuration(this->duration);
=======
void ReflectCardAction::OnExecute() {
  auto& owner = GetUser();

  // Create a new reflect shield component. This handles the logic for shields.
  ReflectShield* reflect = new ReflectShield(&owner, damage);

  // Add the component to the player
  owner.RegisterComponent(reflect);
>>>>>>> menu-refactor

  // Play the appear sound
  Audio().Play(AudioType::APPEAR);

  // Add shield artifact on the same layer as player
<<<<<<< HEAD
  Battle::Tile* tile = user->GetTile();

  if (tile) {
    user->GetField()->AddEntity(*reflect, tile->GetX(), tile->GetY());
  }
}

void ReflectCardAction::SetDuration(const frame_time_t& duration)
{
  this->duration = duration;

  // add override anims
  OverrideAnimationFrames({
    { 1, duration.asSeconds() }
  });
}

void ReflectCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void ReflectCardAction::OnAnimationEnd()
{
}

void ReflectCardAction::EndAction()
{
  Eject();
=======
  Battle::Tile* tile = owner.GetTile();

  if (tile) {
    auto status = owner.GetField()->AddEntity(*reflect, tile->GetX(), tile->GetY());
    if (status != Field::AddEntityStatus::deleted) {
      Entity::RemoveCallback& cleanup = reflect->CreateRemoveCallback();
      cleanup.Slot([this]() {
        this->EndAction();
      });
    }
    else {
      this->EndAction(); // quit early
    }
  }
}

void ReflectCardAction::OnEndAction()
{
>>>>>>> menu-refactor
}
