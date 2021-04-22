#include "bnProgsManThrowState.h"
#include "bnProgsMan.h"
#include "bnProgBomb.h"
#include "bnField.h"

ProgsManThrowState::ProgsManThrowState() : AIState<ProgsMan>()
{
  lastTargetPos = nullptr;
}


ProgsManThrowState::~ProgsManThrowState()
{
}

void ProgsManThrowState::OnEnter(ProgsMan& progs) {
  auto spawnBomb = [this, &progs]() { 
    if (!progs.GetTarget())
      return;

    // Spawn the bomb with the sprite position where Progsman is
    auto startPos = progs.GetTile()->getPosition();
    startPos.y -= 60.0f;
    ProgBomb* bomb = new ProgBomb(progs.GetTeam(), startPos, 1);

    // Assign progsman as the owner of this spell
    auto props = bomb->GetHitboxProperties();
    props.aggressor = progs.GetID();
    bomb->SetHitboxProperties(props);

    // Place the bomb right on the tile
    bomb->SetTile(progs.GetTarget()->GetTile());
    
    // Add the entity into the field
    // NOTE: Is this necessary? 
    progs.GetField()->AddEntity(*bomb, progs.GetTarget()->GetTile()->GetX(), progs.GetTarget()->GetTile()->GetY());
  };

  auto onFinish  = [this, p = &progs]() { p->GoToNextState(); };

  auto anim = progs.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("THROW", onFinish);
  anim->SetCounterFrameRange(1, 2);
  anim->AddCallback(3, spawnBomb, true); // Spawn the bomb on frame 3
}

void ProgsManThrowState::OnLeave(ProgsMan& progs) {

}

void ProgsManThrowState::OnUpdate(double _elapsed, ProgsMan& progs) {
  if(progs.GetTarget() && progs.GetTarget()->GetTile()) {
    lastTargetPos = progs.GetTarget()->GetTile();
  }
}