#include "bnAlphaGunState.h"
#include "bnAlphaIdleState.h"
#include "bnAlphaCore.h"
#include "bnTile.h"
#include "bnField.h"

AlphaGunState::AlphaGunState() : AIState<AlphaCore>(), cooldown(2.83f) { ; }
AlphaGunState::~AlphaGunState() { ; }

void AlphaGunState::OnEnter(AlphaCore& a) {
  a.OpenShoulderGuns();
}

void AlphaGunState::OnUpdate(float _elapsed, AlphaCore& a) {
  cooldown -= _elapsed;

  if (cooldown <= 0) {
    a.ChangeState<AlphaIdleState>();
  }
}

void AlphaGunState::OnLeave(AlphaCore& a) {
  a.CloseShoulderGuns();
}
