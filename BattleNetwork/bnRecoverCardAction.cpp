#include "bnRecoverCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnParticleHeal.h"

#define FRAME1 { 1, 0.1f }

#define FRAMES FRAME1


RecoverCardAction::RecoverCardAction(Character& owner, int heal) : CardAction(owner, "PLAYER_IDLE") {
  RecoverCardAction::heal = heal;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

RecoverCardAction::~RecoverCardAction()
{
}

void RecoverCardAction::OnExecute() {
  auto& owner = GetUser();

  // Increase player health
  owner.SetHealth(owner.GetHealth() + heal);

  // Play sound
  AUDIO.Play(AudioType::RECOVER);

  // Add artifact on the same layer as player
  Battle::Tile* tile = owner.GetTile();

  if (tile) {
    auto healfx = new ParticleHeal();
    auto status = owner.GetField()->AddEntity(*healfx, tile->GetX(), tile->GetY());

    if (status != Field::AddEntityStatus::deleted) {
      Entity::RemoveCallback& cleanup = healfx->CreateRemoveCallback();
      cleanup.Slot([this]() {
        this->EndAction();
      });
    }
    else {
      this->EndAction(); // quit early
    }
  }
}

void RecoverCardAction::OnEndAction()
{
  // do nothing special
}
