#include "bnMetrid.h"
#include "bnMetridIdleState.h"

// Why only MSVC?
#ifndef WIN32
#include "bnMetridMoveState.h"
#else 
#include "bnMetridMoveState.cpp"
#endif

#include "bnAnimationComponent.h"
#include <iostream>

MetridIdleState::MetridIdleState() : cooldown(1), AIState<Metrid>() { ; }
MetridIdleState::~MetridIdleState() { ; }

void MetridIdleState::OnEnter(Metrid& met) {
  auto animation = met.GetFirstComponent<AnimationComponent>();

  if (met.GetRank() == Metrid::Rank::_1) {
    animation->SetAnimation("MOB_IDLE_1");
  } else if (met.GetRank() == Metrid::Rank::_2) {
   animation->SetAnimation("MOB_IDLE_2");
    cooldown = 0.6f;
  }  else if (met.GetRank() == Metrid::Rank::_3) {
    animation->SetAnimation("MOB_IDLE_3");
    cooldown = 0.4f;
  }

  animation->SetPlaybackMode(Animator::Mode::Loop);
}

void MetridIdleState::OnUpdate(float _elapsed, Metrid& met) {
  cooldown -= _elapsed;

  if (cooldown < 0) {
    met.ChangeState<MetridMoveState>();
  }
}

void MetridIdleState::OnLeave(Metrid& met) {
}

