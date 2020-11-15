#include "bnReflectCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define FRAME1 { 1, 1.3f }

#define FRAMES FRAME1


ReflectCardAction::ReflectCardAction(Character * owner, int damage, ReflectShield::Type type) :
  CardAction(*owner, "PLAYER_IDLE"),
  type(type) 
{
  ReflectCardAction::damage = damage;

  // default override anims
  OverrideAnimationFrames({ FRAMES });
}

ReflectCardAction::~ReflectCardAction()
{
}

void ReflectCardAction::Execute() {
  auto user = GetOwner();

  // Create a new reflect shield component. This handles the logic for shields.
  ReflectShield* reflect = new ReflectShield(user, damage, type);
  reflect->SetDuration(this->duration);

  // Play the appear sound
  AUDIO.Play(AudioType::APPEAR);

  // Add shield artifact on the same layer as player
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
}
