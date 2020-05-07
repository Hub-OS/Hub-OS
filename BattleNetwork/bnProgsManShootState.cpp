#include "bnProgsManShootState.h"
#include "bnProgsMan.h"
#include "bnBuster.h"

ProgsManShootState::ProgsManShootState() : AIState<ProgsMan>()
{
}


ProgsManShootState::~ProgsManShootState()
{
}

void ProgsManShootState::OnEnter(ProgsMan& progs) {
  auto spawnBuster = [this, &progs]() {
    Buster* buster = new Buster(progs.GetField(), progs.GetTeam(), false, 10);

    // Spawn a buster aiming down the field
    Direction dir = (progs.GetTeam() == Team::blue)? Direction::left : Direction::right;
    buster->SetDirection(dir);
    buster->SetTile(progs.GetTarget()->GetTile());
    
    // NOTE: Is this necessary anymore
    progs.GetField()->AddEntity(*buster, progs.GetTile()->GetX(), progs.GetTile()->GetY());
  };

  auto onFinish = [this, p = &progs]() { p->GoToNextState(); };

  progs.SetAnimation("SHOOT", onFinish);
  progs.SetCounterFrame(1);
  progs.SetCounterFrame(2);
  progs.OnFrameCallback(3, spawnBuster, Animator::NoCallback, true);
}

void ProgsManShootState::OnLeave(ProgsMan& progs) {

}

void ProgsManShootState::OnUpdate(float _elapsed, ProgsMan& progs) {

}