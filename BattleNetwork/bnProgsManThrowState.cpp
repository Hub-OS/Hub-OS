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
    // Spawn the bomb with the sprite position where Progsman is
    ProgBomb* bomb = new ProgBomb(progs.GetField(), progs.GetTeam(), progs.GetTile()->getPosition(), 1);

    // Assign progsman as the owner of this spell
    auto props = bomb->GetHitboxProperties();
    props.aggressor = &progs;
    bomb->SetHitboxProperties(props);

    // Place the bomb right on the tile
    bomb->SetTile(progs.GetTarget()->GetTile());
    
    // Add the entity into the field
    // NOTE: Is this necessary? 
    progs.GetField()->AddEntity(*bomb, progs.GetTarget()->GetTile()->GetX(), progs.GetTarget()->GetTile()->GetY());
  };

  auto onFinish  = [this, &progs]() { this->ChangeState<ProgsManIdleState>(); };

  progs.SetAnimation(MOB_THROW, onFinish);
  
  // Counter on frame 3
  progs.SetCounterFrame(3);
  
  // Spawn the bomb on frame 4
  progs.OnFrameCallback(4, spawnBomb, std::function<void()>(), true);
}

void ProgsManThrowState::OnLeave(ProgsMan& progs) {

}

void ProgsManThrowState::OnUpdate(float _elapsed, ProgsMan& progs) {

}