#include "bnAlphaGunState.h"
#include "bnAlphaCore.h"
#include "bnSuperVulcan.h"
#include "bnDelayedAttack.h"
#include "bnTile.h"
#include "bnField.h"

AlphaGunState::AlphaGunState() : AIState<AlphaCore>(), cooldown(0.33f) { ; }
AlphaGunState::~AlphaGunState() { ; }

void AlphaGunState::OnEnter(AlphaCore& a) {
  cooldown = 0.13f;
  count = 0;
  a.OpenShoulderGuns();
  last = a.GetTarget()->GetTile();;
}

void AlphaGunState::OnUpdate(float _elapsed, AlphaCore& a) {
  cooldown -= _elapsed;

  if (cooldown <= 0) {
    a.ShootSuperVulcans();

    if (last) {
      auto v = new SuperVulcan(a.GetField(), a.GetTeam(), 40);
      auto d = new DelayedAttack(v, Battle::Tile::Highlight::flash, 0.25);
      a.GetField()->AddEntity(*d, last->GetX(), last->GetY());
    }

    last = a.GetTarget()->GetTile();;

    count++;
    cooldown = 0.23f;

    if (count == 16) {
      a.GoToNextState();
    }
  }
}

void AlphaGunState::OnLeave(AlphaCore& a) {
  a.CloseShoulderGuns();
}
