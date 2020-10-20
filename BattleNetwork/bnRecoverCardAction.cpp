#include "bnRecoverCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnParticleHeal.h"

#define FRAME1 { 1, 0.1f }

#define FRAMES FRAME1


RecoverCardAction::RecoverCardAction(Character * owner, int heal) : CardAction(owner, "PLAYER_IDLE", nullptr, "Origin") {
  RecoverCardAction::heal = heal;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

RecoverCardAction::~RecoverCardAction()
{
}

void RecoverCardAction::Execute() {
  auto user = GetOwner();

  // Increase player health
  user->SetHealth(user->GetHealth() + heal);

  // Play sound
  AUDIO.Play(AudioType::RECOVER);

  // Add artifact on the same layer as player
  Battle::Tile* tile = user->GetTile();

  if (tile) {
    auto healfx = new ParticleHeal();
    user->GetField()->AddEntity(*healfx, tile->GetX(), tile->GetY());
  }
}

void RecoverCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void RecoverCardAction::OnAnimationEnd()
{
}

void RecoverCardAction::EndAction()
{
  Eject();
}
