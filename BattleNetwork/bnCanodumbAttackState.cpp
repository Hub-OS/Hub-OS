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

  auto onAttack = [&can]() { 
    CanonSmoke* smoke = new CanonSmoke(can.GetField());
    can.GetField()->AddEntity(*smoke, can.GetTile()->GetX() - 1, can.GetTile()->GetY()); 

    if (can.GetField()->GetAt(can.tile->GetX() - 1, can.tile->GetY())) {
        Spell* spell = new Cannon(can.field, can.team, 10);
        spell->SetDirection(Direction::left);
        auto props = spell->GetHitboxProperties();
        props.aggressor = &can;
        spell->SetHitboxProperties(props);

        can.field->AddEntity(*spell, can.tile->GetX() - 1, can.tile->GetY());

        can.Audio().Play(AudioType::CANNON);
    }
  };

  auto animation = can.GetFirstComponent<AnimationComponent>();

  switch (can.GetRank()) {
  case Canodumb::Rank::_1:
    animation->SetAnimation("SHOOT_1", onFinish);
    break;
  case Canodumb::Rank::_2:
    animation->SetAnimation("SHOOT_2", onFinish);
    break;
  case Canodumb::Rank::_3:
    animation->SetAnimation("SHOOT_3", onFinish);
    break;
  }

  animation->SetCounterFrameRange(1, 4);
  animation->AddCallback(2, onAttack, true);
}

void CanodumbAttackState::OnUpdate(float _elapsed, Canodumb& can) {
}

void CanodumbAttackState::OnLeave(Canodumb& can) {
}

