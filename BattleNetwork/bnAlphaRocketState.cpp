#include "bnAlphaRocketState.h"
#include "bnAlphaCore.h"
#include "bnAnimationComponent.h"
#include "bnAlphaRocket.h"
#include "bnTile.h"
#include "bnField.h"

AlphaRocketState::AlphaRocketState() : AIState<AlphaCore>() { ; }
AlphaRocketState::~AlphaRocketState() { ; }

void AlphaRocketState::OnEnter(AlphaCore& a) {
  // skip unless at half health
  if (a.GetHealth() > a.GetMaxHealth() / 2) {
    //a.GoToNextState();
    //return;
  }

  launched = false;

  a.EnableImpervious();
  AnimationComponent* anim = a.GetFirstComponent<AnimationComponent>();

  auto onFinish = [alpha = &a, anim, this]() {
    alpha->EnableImpervious(false);
    alpha->GoToNextState();

    AlphaRocket* rocket = new AlphaRocket(alpha->GetTeam());
    auto props = rocket->GetHitboxProperties();
    props.aggressor = alpha->GetID();
    rocket->SetHitboxProperties(props);

    rocket->SetDirection(Direction::left);

    // This will happen on the core's tick step but we want it to happen immediately after launching too.
    anim->SetAnimation("CORE_FULL");
    anim->SetPlaybackMode(Animator::Mode::Loop);

    launched = true;

    alpha->GetField()->AddEntity(*rocket, alpha->GetTile()->GetX() - 1, alpha->GetTile()->GetY());
  };

  anim->SetAnimation("CORE_ATTACK2", onFinish);
}

void AlphaRocketState::OnUpdate(double _elapsed, AlphaCore& a) {
  if (launched) {
    a.GoToNextState();
  }
}

void AlphaRocketState::OnLeave(AlphaCore& a) {
}
