#include "bnAlphaGunState.h"
#include "bnAlphaCore.h"
#include "bnTile.h"
#include "bnField.h"

AlphaGunState::AlphaGunState() : AIState<AlphaCore>(), cooldown(2.83f) { ; }
AlphaGunState::~AlphaGunState() { ; }

void AlphaGunState::OnEnter(AlphaCore& a) {
  cooldown = 2.83f;
  a.OpenShoulderGuns();
}

void AlphaGunState::OnUpdate(float _elapsed, AlphaCore& a) {
  cooldown -= _elapsed;

  if (cooldown <= 0) {
    a.GoToNextState();
  }
}

void AlphaGunState::OnLeave(AlphaCore& a) {
  a.CloseShoulderGuns();
}
