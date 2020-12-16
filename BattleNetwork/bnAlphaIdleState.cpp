#include "bnAlphaIdleState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnAlphaCore.h"

AlphaIdleState::AlphaIdleState() : AIState<AlphaCore>(), cooldown(2.83f) { ; }
AlphaIdleState::~AlphaIdleState() { ; }

void AlphaIdleState::OnEnter(AlphaCore& a) {
  cooldown = 2.83;
}

void AlphaIdleState::OnUpdate(double _elapsed, AlphaCore& a) {
  cooldown -= _elapsed;

  if (cooldown <= 0) {
    a.GoToNextState();
  }
}

void AlphaIdleState::OnLeave(AlphaCore& a) {
}
