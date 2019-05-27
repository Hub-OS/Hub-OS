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
<<<<<<< HEAD
    buster->SetDirection(Direction::LEFT);
    buster->SetTile(progs.GetTarget()->GetTile());

=======
    
    // Spawn a buster aiming down the field
    Direction dir = (progs.GetTeam() == Team::BLUE)? Direction::LEFT : Direction::RIGHT;
    buster->SetDirection(dir);
    buster->SetTile(progs.GetTarget()->GetTile());
    
    // NOTE: Is this necessary anymore
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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