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
    Buster* buster = new Buster(progs.GetField(), progs.GetTeam(), false);
    buster->SetDirection(Direction::LEFT);
    buster->SetTile(progs.GetTarget()->GetTile());

    progs.GetField()->AddEntity(*buster, progs.GetTile()->GetX(), progs.GetTile()->GetY());
  };

  auto onFinish = [this, &progs]() { this->ChangeState<ProgsManIdleState>(); };

  progs.SetAnimation("SHOOT", onFinish);
  progs.SetCounterFrame(1);
  progs.SetCounterFrame(2);
  progs.OnFrameCallback(3, spawnBuster, std::function<void()>(), true);
}

void ProgsManShootState::OnLeave(ProgsMan& progs) {

}

void ProgsManShootState::OnUpdate(float _elapsed, ProgsMan& progs) {

}