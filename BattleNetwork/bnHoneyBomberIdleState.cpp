#include "bnHoneyBomber.h"
#include "bnHoneyBomberIdleState.h"
#include "bnHoneyBomberMoveState.h"
#include "bnAnimationComponent.h"
#include <iostream>

HoneyBomberIdleState::HoneyBomberIdleState() : cooldown(1), AIState<HoneyBomber>() { ; }
HoneyBomberIdleState::~HoneyBomberIdleState() { ; }

void HoneyBomberIdleState::OnEnter(HoneyBomber& honey) {
  auto animation = honey.GetFirstComponent<AnimationComponent>();

  animation->SetAnimation("IDLE");

  animation->SetPlaybackMode(Animator::Mode::Loop);
}

void HoneyBomberIdleState::OnUpdate(double _elapsed, HoneyBomber& honey) {
  cooldown -= _elapsed;

  if (cooldown < 0) {
    honey.ChangeState<HoneyBomberMoveState>();
  }
}

void HoneyBomberIdleState::OnLeave(HoneyBomber& honey) {
}

