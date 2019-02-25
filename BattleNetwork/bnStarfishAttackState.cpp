#pragma once
#include "bnStarfishIdleState.h"
#include "bnBubble.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnStarfishAttackState.h"

StarfishAttackState::StarfishAttackState(int maxBubbleCount) : bubbleCount(maxBubbleCount), AIState<Starfish>() { ; }
StarfishAttackState::~StarfishAttackState() { ; }

void StarfishAttackState::OnEnter(Starfish& star) {
  auto onFinish = [this, &star]() {this->DoAttack(star); };

  star.SetAnimation("ATTACK");

  star.animationComponent.AddCallback(13, onFinish, std::function<void()>(), true);

  star.SetCounterFrame(10);
  star.SetCounterFrame(11);
  star.SetCounterFrame(12);
}

void StarfishAttackState::OnUpdate(float _elapsed, Starfish& star) {
  /* Nothing, just wait the animation out*/
}

void StarfishAttackState::OnLeave(Starfish& star) {
}

void StarfishAttackState::DoAttack(Starfish& star) {
  if (star.GetField()->GetAt(star.GetTile()->GetX() - 1, star.GetTile()->GetY())->IsWalkable()) {
    Spell* spell = new Bubble(star.field, star.team, (star.GetRank() == Starfish::Rank::SP) ? 1.5 : 1.0);
    spell->SetDirection(Direction::LEFT);
    star.field->AddEntity(*spell, star.tile->GetX() - 1, star.tile->GetY());
  }

  if (--bubbleCount > 0) {
    this->ChangeState<StarfishAttackState>(bubbleCount);
  }
  else {
    this->ChangeState<StarfishIdleState>();
  }
}
