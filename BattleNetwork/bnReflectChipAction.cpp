#include "bnReflectChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnReflectShield.h"

#define FRAME1 { 1, 2 }

#define FRAMES FRAME1


ReflectChipAction::ReflectChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_IDLE", nullptr, "Buster") {
  // add override anims
  this->OverrideAnimationFrames({ FRAMES });

  // Create a new reflect shield component. This handles the logic for shields.
  ReflectShield* reflect = new ReflectShield(owner, damage);

  // Add the component to the player
  owner->RegisterComponent(reflect);

  // Play the appear sound
  AUDIO.Play(AudioType::APPEAR);

  // Add shield artifact on the same layer as player
  Battle::Tile* tile = owner->GetTile();

  if (tile) {
    owner->GetField()->AddEntity(*reflect, tile->GetX(), tile->GetY());
  }
}

ReflectChipAction::~ReflectChipAction()
{
}

void ReflectChipAction::OnUpdate(float _elapsed)
{
  ChipAction::OnUpdate(_elapsed);
}

void ReflectChipAction::EndAction()
{
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
