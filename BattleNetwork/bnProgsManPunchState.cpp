#include "bnProgsManPunchState.h"
#include "bnProgsMan.h"

ProgsManPunchState::ProgsManPunchState() : AIState<ProgsMan>()
{
}


ProgsManPunchState::~ProgsManPunchState()
{
}

void ProgsManPunchState::OnEnter(ProgsMan& progs) {
  auto onFinish = [this, &progs]() { this->Attack(progs); };
  progs.SetAnimation(MOB_ATTACKING, onFinish);
  progs.SetCounterFrame(1);
  progs.SetCounterFrame(2);
  progs.SetCounterFrame(3);
}

void ProgsManPunchState::OnLeave(ProgsMan& progs) {
}

void ProgsManPunchState::OnUpdate(float _elapsed, ProgsMan& progs) {

}

void ProgsManPunchState::Attack(ProgsMan& progs) {
  Battle::Tile* tile = progs.GetTile();
  if (tile->GetX() - 1 >= 1) {

    Battle::Tile* next = 0;
    next = progs.GetField()->GetAt(tile->GetX() - 1, tile->GetY());

    Entity* entity = 0;

    while (next->GetNextEntity(entity)) {
      Character* isCharacter = dynamic_cast<Character*>(entity);

      if (isCharacter && isCharacter->GetTeam() != progs.GetTeam()) {
        if (isCharacter->Hit(20)) {
          isCharacter->SlideToTile(true);
          isCharacter->Move(Direction::LEFT);
        }
      }
    }
  }

  tile = 0;

  this->ChangeState<ProgsManIdleState>();
}