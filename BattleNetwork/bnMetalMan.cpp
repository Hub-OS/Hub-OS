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
    AddState<MetalManPunchState>();
    AddState<MetalManMoveState>();
    AddState<MetalManPunchState>();
    AddState<MetalManMoveState>();
    AddState<MetalManPunchState>();
    AddState<MetalManMissileState>(10);
    AddState<MetalManIdleState>();
  }
  else {
    SetHealth(100);
  }

  ShareTileSpace(true); // mega can walk into him on red tiles

  SetHeight(96.f);

  healthUI = new MobHealthUI(this);

  setTexture(Textures().GetTexture(TextureType::MOB_METALMAN_ATLAS));

  setScale(2.f, 2.f);

  SetHealth(health);
  SetFloatShoe(true);

  //Components setup and load
  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("IDLE");
  animationComponent->SetPlaybackMode(Animator::Mode::Loop);

  animationComponent->OnUpdate(0);

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);

  auto stun = [this]() {
    if (!Teammate(GetTile()->GetTeam())) {
      MoveEvent event = { 0, 0, frames(3), 0, GetField()->GetAt(6, 2) };
      actionQueue.Add(event, ActionOrder::immediate, ActionDiscardOp::until_resolve);
    }
  };

  RegisterStatusCallback(Hit::stun, stun);
}

MetalMan::~MetalMan() {
  delete virusBody;
}

bool MetalMan::CanMoveTo(Battle::Tile * next)
{
  if (!next->ContainsEntityType<Character>() && !next->ContainsEntityType<Obstacle>() && !next->IsEdgeTile()) {
    if (next->GetTeam() != GetTeam() && canEnterRedTeam) {
      return true;
    }
    else {
      return next->GetTeam() == GetTeam();
    }
  }

  return false;
}

void MetalMan::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  BossPatternAI<MetalMan>::Update(_elapsed);

  Hitbox* hitbox = new Hitbox(GetTeam(), 40);
  auto props = hitbox->GetHitboxProperties();
  props.flags |= Hit::impact | Hit::recoil | Hit::flinch;
  hitbox->SetHitboxProperties(props);

  field->AddEntity(*hitbox, tile->GetX(), tile->GetY());
}

void MetalMan::OnDelete() {
  InterruptState<NaviExplodeState<MetalMan>>(10, 0.9); // freezes animation
}
