#include "bnProgsManPunchState.h"
#include "bnProgsMan.h"
#include "bnHitBox.h"

ProgsManPunchState::ProgsManPunchState() : AIState<ProgsMan>()
{
}


ProgsManPunchState::~ProgsManPunchState()
{
}

void ProgsManPunchState::OnEnter(ProgsMan& progs) {
  auto onPunch = [this, &progs]() { this->Attack(progs); };
  progs.SetAnimation(MOB_ATTACKING, [p = &progs] { p->ChangeState<ProgsManIdleState>(); });
  progs.GetFirstComponent<AnimationComponent>()->AddCallback(4, onPunch);
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

    if (next) {
      // Spawn a hurt box 
      HitBox* hitbox = new HitBox(progs.GetField(), progs.GetTeam(), 100);
      auto props = hitbox->GetHitboxProperties();
      props.flags = props.flags | Hit::breaking | Hit::flinch;
      props.aggressor = &progs;
      props.flags |= Hit::drag;
      props.drag = Direction::LEFT;
      hitbox->SetHitboxProperties(props);

      progs.GetField()->AddEntity(*hitbox, next->GetX(), next->GetY());
    }
  }

  tile = 0;
}