#include "bnProgsMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnProgBomb.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnGame.h"
#include "bnNaviExplodeState.h"

#define RESOURCE_PATH "resources/mobs/progsman/progsman.animation"

ProgsMan::ProgsMan(Rank _rank) : BossPatternAI<ProgsMan>(this), Character(_rank) {
  name = "ProgsMan";
  team = Team::blue;
  SetHealth(1200);

  if (_rank == ProgsMan::Rank::EX) {
    SetHealth(2500);
  }

  setTexture(Textures().GetTexture(TextureType::MOB_PROGSMAN_ATLAS));
  setScale(2.f, 2.f);

  SetHealth(health);

  //Components setup and load

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("IDLE");
  animationComponent->OnUpdate(0);

  // Setup AI pattern
  AddState<ProgsManIdleState>(1.0f);
  AddState<ProgsManMoveState>();
  AddState<ProgsManIdleState>(1.0f);
  AddState<ProgsManThrowState>();
  AddState<ProgsManIdleState>(1.0f);
  AddState<ProgsManThrowState>();
  AddState<ProgsManIdleState>(1.0f);
  AddState<ProgsManThrowState>();
  AddState<ProgsManIdleState>(2.0f);
  AddState<ProgsManMoveState>();
  AddState<ProgsManShootState>();
  AddState<ProgsManMoveState>();
  AddState<ProgsManShootState>();

  auto recoil = [this]() {
    animationComponent->SetAnimation("MOB_HIT");
    FinishMove();
    InterruptState<ProgsManHitState>();
  };

  RegisterStatusCallback(Hit::flinch, Callback<void()>{recoil});
  RegisterStatusCallback(Hit::stun, Callback<void()>{recoil}); // TODO: should stun auto trigger this?
}

ProgsMan::~ProgsMan() {
}

void ProgsMan::OnUpdate(double _elapsed) {
  BossPatternAI<ProgsMan>::Update(_elapsed);
}

void ProgsMan::OnDelete() {
  animationComponent->SetAnimation("MOB_HIT");
  InterruptState<NaviExplodeState<ProgsMan>>(); // freezes animation
}

const float ProgsMan::GetHeight() const {
  return 64.0f;
}
