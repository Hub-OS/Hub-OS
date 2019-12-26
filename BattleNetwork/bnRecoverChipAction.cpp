#include "bnRecoverChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnParticleHeal.h"

#define FRAME1 { 1, 0.1f }

#define FRAMES FRAME1


RecoverChipAction::RecoverChipAction(Character * owner, int heal) : ChipAction(owner, "PLAYER_IDLE", nullptr, "Origin") {
  this->heal = heal;

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

RecoverChipAction::~RecoverChipAction()
{
}

void RecoverChipAction::Execute() {
  auto owner = GetOwner();

  // Increase player health
  owner->SetHealth(owner->GetHealth() + heal);

  // Play sound
  AUDIO.Play(AudioType::RECOVER);

  // Add artifact on the same layer as player
  Battle::Tile* tile = owner->GetTile();

  if (tile) {
    auto healfx = new ParticleHeal();
    owner->GetField()->AddEntity(*healfx, tile->GetX(), tile->GetY());
  }
}

void RecoverChipAction::OnUpdate(float _elapsed)
{
  ChipAction::OnUpdate(_elapsed);
}

void RecoverChipAction::EndAction()
{
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
