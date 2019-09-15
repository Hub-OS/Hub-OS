#include "bnAlphaIdleState.h"
#include "bnAlphaClawSwipeState.h"
#include "bnTile.h"
#include "bnField.h"

AlphaIdleState::AlphaIdleState() : AIState<AlphaCore>(), cooldown(2.83f) { ; }
AlphaIdleState::~AlphaIdleState() { ; }

void AlphaIdleState::OnEnter(AlphaCore& a) {
}

void AlphaIdleState::OnUpdate(float _elapsed, AlphaCore& a) {
  cooldown -= _elapsed;

  if (cooldown <= 0) {
    this->ChangeState<AlphaClawSwipeState>();
  }
}

void AlphaIdleState::OnLeave(AlphaCore& a) {
}
