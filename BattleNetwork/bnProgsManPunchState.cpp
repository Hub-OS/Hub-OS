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

    if (next) {
      // Get all the characters on the tile
      auto characters = next->FindEntities([](Entity* in) { return dynamic_cast<Character*>(in); });
 
      // Spawn a hurt box 
      for (int i = 0; i < characters.size(); i++) {
        if (characters[i]->GetTeam() != progs.GetTeam()) {
          HitBox* hitbox = new HitBox(progs.GetField(), progs.GetTeam(), 20);
          auto props = hitbox->GetHitboxProperties();
          props.flags = props.flags | Hit::breaking;
          props.aggressor = &progs;
          hitbox->SetHitboxProperties(props);

          progs.GetField()->AddEntity(*hitbox, next->GetX(), next->GetY());

          props.flags = Hit::none;
          props.damage = 0;

          // push every single character 
          if ((dynamic_cast<Character*>(characters[i]))->Hit(props)) {
            characters[i]->SlideToTile(true);
            characters[i]->Move(Direction::LEFT);
          }
        }
      }
    }
  }

  tile = 0;

  this->ChangeState<ProgsManIdleState>();
}