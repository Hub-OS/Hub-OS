#include "bnMetalMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnGame.h"
#include "bnNaviExplodeState.h"
#include "bnMetalManMissileState.h"
#include "bnMetalManMoveState.h"
#include "bnMetalManPunchState.h"
#include "bnMetalManThrowState.h"
#include "bnObstacle.h"
#include "bnDefenseVirusBody.h"
#include "bnHitbox.h"

#define RESOURCE_PATH "resources/mobs/metalman/metalman.animation"

MetalMan::MetalMan(Rank _rank)
  :
  BossPatternAI<MetalMan>(this), Character(_rank) {
  name = "MetalMan";
  team = Team::blue;

  AddState<MetalManIdleState>();
  AddState<MetalManMoveState>();
  AddState<MetalManIdleState>();
  AddState<MetalManMoveState>();
  AddState<MetalManIdleState>();
  AddState<MetalManMoveState>();
  AddState<MetalManThrowState>();
  AddState<MetalManPunchState>();
  AddState<MetalManIdleState>();

  if (rank == Rank::EX) {
    SetHealth(1300);

    // Append more states
    AddState<MetalManMissileState>(10);
    AddState<MetalManIdleState>();
    AddState<MetalManIdleState>();
    AddState<MetalManMoveState>();
    AddState<MetalManMoveState>();
    AddState<MetalManMoveState>();
    AddState<MetalManThrowState>();
    AddState<MetalManMoveState>();
    AddState<MetalManPunchState>();
    AddState<MetalManMoveState>();
    AddState<MetalManPunchState>();
    AddState<MetalManMoveState>();
    AddState<MetalManPunchState>();
    AddState<MetalManMissileState>(10);
    AddState<MetalManIdleState>();
  }
  else {
    SetHealth(1000);
  }

  ShareTileSpace(true); // mega can walk into him on red tiles

  SetHeight(96.f);

  setTexture(Textures().GetTexture(TextureType::MOB_METALMAN_ATLAS));

  setScale(2.f, 2.f);

  SetHealth(health);
  SetFloatShoe(true);

  //Components setup and load
  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("IDLE");
  animationComponent->SetPlaybackMode(Animator::Mode::Loop);

  animationComponent->OnUpdate(0);

  virusBody = std::make_shared<DefenseVirusBody>();
  AddDefenseRule(virusBody);

  auto stun = [this]() {
    if (!Teammate(GetTile()->GetTeam())) {
      AdoptTile(GetField()->GetAt(6, 2));
    }
  };

  RegisterStatusCallback(Hit::stun, stun);
}

bool MetalMan::CanMoveTo(Battle::Tile * next)
{
  if (!next->IsEdgeTile()) {
    if (next->GetTeam() != GetTeam() && canEnterRedTeam) {
      return !next->ContainsEntityType<Character>() && !next->ContainsEntityType<Obstacle>();
    }
    else {
      return next->GetTeam() == GetTeam();
    }
  }

  return false;
}

void MetalMan::OnUpdate(double _elapsed) {
  BossPatternAI<MetalMan>::Update(_elapsed);

  auto hitbox = std::make_shared<Hitbox>(GetTeam(), 40);
  auto props = hitbox->GetHitboxProperties();
  props.flags |= Hit::impact | Hit::flinch | Hit::flash;
  hitbox->SetHitboxProperties(props);

  field->AddEntity(hitbox, tile->GetX(), tile->GetY());
}

void MetalMan::OnDelete() {
  InterruptState<NaviExplodeState<MetalMan>>(10, 0.9); // freezes animation
}
