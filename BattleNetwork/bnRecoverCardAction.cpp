#include "bnRecoverCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnParticleHeal.h"

#define FRAME1 { 1, 0.1f }

#define FRAMES FRAME1


RecoverCardAction::RecoverCardAction(Character& actor, int heal) : 
  CardAction(actor, "PLAYER_IDLE") {
  RecoverCardAction::heal = heal;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

RecoverCardAction::~RecoverCardAction()
{
}

void RecoverCardAction::OnExecute(Character* user) {
  // Increase player health
  user->SetHealth(user->GetHealth() + heal);

  // Play sound
  Audio().Play(AudioType::RECOVER);

  // Add artifact on the same layer as player
  Battle::Tile* tile = user->GetTile();

  if (tile) {
    auto healfx = new ParticleHeal();
    user->GetField()->AddEntity(*healfx, *tile);
  }
}

void RecoverCardAction::OnAnimationEnd()
{
}

void RecoverCardAction::OnEndAction()
{
}
