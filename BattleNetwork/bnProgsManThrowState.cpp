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
    // Spawn the bomb with the sprite position where Progsman is
    auto startPos = progs.GetTile()->getPosition();
    startPos.y -= 60.0f;
    ProgBomb* bomb = new ProgBomb(progs.GetField(), progs.GetTeam(), startPos, 1);

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

  auto onFinish  = [this, p = &progs]() { p->ChangeState<ProgsManIdleState>(); };

  progs.SetAnimation(MOB_THROW, onFinish);
  
  // Counter on frame [1,2]
  progs.SetCounterFrame(1);
  progs.SetCounterFrame(2);
  
  // Spawn the bomb on frame 3
  progs.OnFrameCallback(3, spawnBomb, std::function<void()>(), true);
}

void ProgsManThrowState::OnLeave(ProgsMan& progs) {

}

void ProgsManThrowState::OnUpdate(float _elapsed, ProgsMan& progs) {
  if(progs.GetTarget() && progs.GetTarget()->GetTile()) {
    lastTargetPos = progs.GetTarget()->GetTile();
  }
}