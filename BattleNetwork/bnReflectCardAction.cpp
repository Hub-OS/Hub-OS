#include "bnReflectCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define FRAME1 { 1, 1.3f }

#define FRAMES FRAME1

ReflectCardAction::ReflectCardAction(Character& owner, int damage, ReflectShield::Type type) :
  CardAction(owner, "PLAYER_IDLE"),
  type(type) 
{
  ReflectCardAction::damage = damage;

  // default override anims
  OverrideAnimationFrames({ FRAMES });
}

ReflectCardAction::~ReflectCardAction()
{
}

void ReflectCardAction::OnExecute() {
  auto user = GetOwner();

  // Create a new reflect shield component. This handles the logic for shields.
  ReflectShield* reflect = new ReflectShield(user, damage, type);
  reflect->SetDuration(this->duration);
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

void ReflectCardAction::OnEndAction()
{
  Eject();
}