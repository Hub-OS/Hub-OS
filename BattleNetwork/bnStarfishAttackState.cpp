#pragma once
#include "bnStarfishIdleState.h"
#include "bnBubble.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnStarfishAttackState.h"

StarfishAttackState::StarfishAttackState(int maxBubbleCount) : bubbleCount(maxBubbleCount), AIState<Starfish>() { ; }
StarfishAttackState::~StarfishAttackState() { ; }

void StarfishAttackState::OnEnter(Starfish& star) {
  auto onPreAttack = [this, &star]() { 
    auto onFinish = [this, &star]() {this->DoAttack(star); };

    star.animationComponent.SetAnimation("ATTACK", Animate::Mode::Loop);
    star.animationComponent.AddCallback(5, onFinish, std::function<void()>(), false);

  };


  star.SetAnimation("PREATTACK", onPreAttack);

  star.SetCounterFrame(1);
  star.SetCounterFrame(2);
}

void StarfishAttackState::OnUpdate(float _elapsed, Starfish& star) {
  /* Nothing, just wait the animation out*/
}

void StarfishAttackState::OnLeave(Starfish& star) {
}

void StarfishAttackState::DoAttack(Starfish& star) {
  if (star.GetField()->GetAt(star.GetTile()->GetX() - 1, star.GetTile()->GetY())->IsWalkable()) {
    Spell* spell = new Bubble(star.field, star.team, (star.GetRank() == Starfish::Rank::SP) ? 1.5 : 1.0);
    spell->SetHitboxProperties({ 40, spell->GetHitboxProperties().flags | Hit::impact, Element::AQUA, 3.0, &star });
    spell->SetDirection(Direction::LEFT);
    star.field->AddEntity(*spell, star.tile->GetX() - 1, star.tile->GetY());
  }

  if (--bubbleCount > 0) {
    //this->ChangeState<StarfishAttackState>(bubbleCount);
  }
  else {
    this->ChangeState<StarfishIdleState>();
  }
}
