#include "bnProgsManPunchState.h"
#include "bnProgsMan.h"
#include "bnHitbox.h"
#include "bnField.h"

ProgsManPunchState::ProgsManPunchState() : AIState<ProgsMan>()
{
}


ProgsManPunchState::~ProgsManPunchState()
{
}

void ProgsManPunchState::OnEnter(ProgsMan& progs) {
  auto onPunch = [this, &progs]() { Attack(progs); };
  auto onPunchFinish = [p = &progs] { p->GoToNextState(); };

  auto anim = progs.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("ATTACKING", onPunchFinish);
  anim->AddCallback(4, onPunch);
  anim->SetCounterFrameRange(1, 3);
}

void ProgsManPunchState::OnLeave(ProgsMan& progs) {
}

void ProgsManPunchState::OnUpdate(double _elapsed, ProgsMan& progs) {

}

void ProgsManPunchState::Attack(ProgsMan& progs) {
  Battle::Tile* tile = progs.GetTile();
  if (tile->GetX() - 1 >= 1) {
    Battle::Tile* next = 0;
    next = progs.GetField()->GetAt(tile->GetX() - 1, tile->GetY());

    if (next) {
      // Spawn a hurt box
      Hitbox* hitbox = new Hitbox(progs.GetTeam(), 100);
      auto props = hitbox->GetHitboxProperties();
      props.flags = props.flags | Hit::breaking | Hit::flinch;
      props.aggressor = &progs;
      props.flags |= Hit::drag;
      props.drag = Direction::left;
      hitbox->SetHitboxProperties(props);

      progs.GetField()->AddEntity(*hitbox, next->GetX(), next->GetY());
    }
  }

  tile = 0;
}
