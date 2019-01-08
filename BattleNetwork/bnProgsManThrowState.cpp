#include "bnProgsManThrowState.h"
#include "bnProgsMan.h"
#include "bnProgBomb.h"

ProgsManThrowState::ProgsManThrowState() : AIState<ProgsMan>()
{
}


ProgsManThrowState::~ProgsManThrowState()
{
}

void ProgsManThrowState::OnEnter(ProgsMan& progs) {
  auto spawnBomb = [this, &progs]() { 
    ProgBomb* bomb = new ProgBomb(progs.GetField(), progs.GetTeam(), progs.GetTarget()->GetTile(), 1);
    bomb->SetTile(progs.GetTile());
    bomb->PrepareThrowPath();

    progs.GetField()->OwnEntity(bomb, progs.GetTile()->GetX(), progs.GetTile()->GetY()); 
  };

  auto onFinish  = [this, &progs]() { this->ChangeState<ProgsManIdleState>(); };

  progs.SetAnimation(MOB_THROW, onFinish);
  progs.SetCounterFrame(2);
  progs.OnFrameCallback(4, spawnBomb, std::function<void()>(), true);
}

void ProgsManThrowState::OnLeave(ProgsMan& progs) {

}

void ProgsManThrowState::OnUpdate(float _elapsed, ProgsMan& progs) {

}