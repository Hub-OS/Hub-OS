#include "bnCanodumbIdleState.h"
#include "bnCanodumbAttackState.h"
#include "bnCanodumb.h"
#include "bnAudioResourceManager.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnCannon.h"
#include <iostream>

#include "bnCanonSmoke.h"

CanodumbAttackState::CanodumbAttackState() : AIState<Canodumb>() { ; }
CanodumbAttackState::~CanodumbAttackState() { ; }

void CanodumbAttackState::OnEnter(Canodumb& can) {
  auto onFinish = [&can]() { can.ChangeState<CanodumbIdleState>(); };

  auto spawnSmoke   = [&can]() { 
    CanonSmoke* smoke = new CanonSmoke(can.GetField(), can.GetTeam());
    can.GetField()->OwnEntity(smoke, can.GetTile()->GetX() - 1, can.GetTile()->GetY()); 
  };

  switch (can.GetRank()) {
  case Canodumb::Rank::_1:
    can.SetAnimation(MOB_CANODUMB_SHOOT_1, onFinish);
    break;
  case Canodumb::Rank::_2:
    can.SetAnimation(MOB_CANODUMB_SHOOT_2, onFinish);
    break;
  case Canodumb::Rank::_3:
    can.SetAnimation(MOB_CANODUMB_SHOOT_3, onFinish);
    break;
  }

  can.SetCounterFrame(2);
  can.OnFrameCallback(1, spawnSmoke, std::function<void()>(), true);
}

void CanodumbAttackState::OnUpdate(float _elapsed, Canodumb& can) {
}

void CanodumbAttackState::OnLeave(Canodumb& can) {
  // TODO: Make this a frame-dependant callback
  if (can.GetField()->GetAt(can.tile->GetX() - 1, can.tile->GetY())) {
    Spell* spell = new Cannon(can.field, can.team, 10);
    spell->SetDirection(Direction::LEFT);
    can.field->OwnEntity(spell, can.tile->GetX() - 1, can.tile->GetY());

    AUDIO.Play(AudioType::CANNON);
  }

}

