#include "bnMetalMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEngine.h"
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
    SetHealth(1000);
  }

  ShareTileSpace(true); // mega can walk into him on red tiles
  
  hitHeight = 64;
  SetHeight(hitHeight);

  healthUI = new MobHealthUI(this);

  setTexture(TEXTURES.GetTexture(TextureType::MOB_METALMAN_ATLAS));

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

  // TODO: take this out
  // multi-move attacks (like punch) will have a destination tile
  movedByStun = false;

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);
}

MetalMan::~MetalMan() {
    delete virusBody;
}

void MetalMan::OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave, bool doOnce) {
  animationComponent->AddCallback(frame, onEnter, onLeave, doOnce);
}

bool MetalMan::CanMoveTo(Battle::Tile * next)
{
  if (!next->ContainsEntityType<Character>() && !next->ContainsEntityType<Obstacle>() && !next->IsEdgeTile()) {
    return true;
  }

  return false;
}

void MetalMan::OnUpdate(float _elapsed) {
  // TODO: use StuntDoubles to circumvent teleportaton
  if (movedByStun) { 
    Teleport((rand() % 3) + 4, (rand() % 3) + 1); 
    AdoptNextTile(); 
    FinishMove();
    movedByStun = false; 
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  BossPatternAI<MetalMan>::Update(_elapsed);

  Hitbox* hitbox = new Hitbox(GetField(), GetTeam(), 40);
  auto props = hitbox->GetHitboxProperties();
  props.flags |= Hit::impact | Hit::recoil | Hit::flinch;
  hitbox->SetHitboxProperties(props);

  field->AddEntity(*hitbox, tile->GetX(), tile->GetY());
}

const bool MetalMan::OnHit(const Hit::Properties props) {
  bool result = true;

  if ((props.flags & Hit::stun) == Hit::stun) {
    if (!Teammate(GetTile()->GetTeam())) {
      movedByStun = true;
    }
  }

  return result;
}

void MetalMan::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { ToggleCounter(); };
  auto onNext = [&]() { ToggleCounter(false); };
  animationComponent->AddCallback(frame, onFinish, onNext);
}

void MetalMan::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;
  animationComponent->SetAnimation(_state, onFinish);
  animationComponent->OnUpdate(0);
}

void MetalMan::OnDelete() {
  InterruptState<NaviExplodeState<MetalMan>>(); // freezes animation
}