#include "bnProgsManShootState.h"
#include "bnProgsMan.h"
#include "bnBuster.h"
#include "bnField.h"

ProgsManShootState::ProgsManShootState() : AIState<ProgsMan>()
{
}


ProgsManShootState::~ProgsManShootState()
{
}

void ProgsManShootState::OnEnter(ProgsMan& progs) {
  auto spawnBuster = [this, &progs]() {
    Buster* buster = new Buster(progs.GetTeam(), false, 10);

    // Spawn a buster aiming down the field
    Direction dir = (progs.GetTeam() == Team::blue)? Direction::left : Direction::right;
    buster->SetMoveDirection(dir);
    buster->SetTile(progs.GetTarget()->GetTile());
    
    // NOTE: Is this necessary anymore
    progs.GetField()->AddEntity(*buster, *progs.GetTile());
  };

  auto onFinish = [this, p = &progs]() { p->GoToNextState(); };

  auto anim = progs.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("SHOOT", onFinish);
  anim->SetCounterFrameRange(1, 2);
  anim->AddCallback(3, spawnBuster, true);
}

void ProgsManShootState::OnLeave(ProgsMan& progs) {

}

void ProgsManShootState::OnUpdate(double _elapsed, ProgsMan& progs) {

}