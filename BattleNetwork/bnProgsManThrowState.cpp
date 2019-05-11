#include "bnProgsManThrowState.h"
#include "bnProgsMan.h"
#include "bnProgBomb.h"

ProgsManThrowState::ProgsManThrowState() : AIState<ProgsMan>()
{
  lastTargetPos = nullptr;
}


ProgsManThrowState::~ProgsManThrowState()
{
}

void ProgsManThrowState::OnEnter(ProgsMan& progs) {
  auto spawnBomb = [this, &progs]() {
    if(lastTargetPos) {
      ProgBomb* bomb = new ProgBomb(progs.GetField(), progs.GetTeam(), progs.GetTile()->getPosition(), 1);

      auto props = bomb->GetHitboxProperties();
      props.aggressor = &progs;
      bomb->SetHitboxProperties(props);

      bomb->SetTile(lastTargetPos);

      progs.GetField()->AddEntity(*bomb, lastTargetPos->GetX(), lastTargetPos->GetY());
    }
  };

  auto onFinish  = [this, &progs]() { this->ChangeState<ProgsManIdleState>(); };

  progs.SetAnimation(MOB_THROW, onFinish);
  progs.SetCounterFrame(3);
  progs.OnFrameCallback(4, spawnBomb, std::function<void()>(), true);
}

void ProgsManThrowState::OnLeave(ProgsMan& progs) {

}

void ProgsManThrowState::OnUpdate(float _elapsed, ProgsMan& progs) {
  if(progs.GetTarget() && progs.GetTarget()->GetTile()) {
    lastTargetPos = progs.GetTarget()->GetTile();
  }
}