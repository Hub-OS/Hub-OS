#include "bnStarfishIdleState.h"
#include "bnBubble.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnStarfishAttackState.h"

StarfishAttackState::StarfishAttackState(int maxBubbleCount) : bubbleCount(maxBubbleCount), AIState<Starfish>() { 
	leaveState = false; 
}

StarfishAttackState::~StarfishAttackState() { ; }

void StarfishAttackState::OnEnter(Starfish& star) {
  auto onPreAttack = [this, s = &star]() { 

    auto onAttack = [this, s]() {
      this->DoAttack(*s); 
    };
	
    s->GetFirstComponent<AnimationComponent>()->SetAnimation("ATTACK", Animator::Mode::Loop);
    s->OnFrameCallback(1, onAttack, Animator::NoCallback, false);
  };


  star.SetAnimation("PREATTACK", onPreAttack);

  // star.SetCounterFrame(1);
}

void StarfishAttackState::OnUpdate(float _elapsed, Starfish& star) {
}

void StarfishAttackState::OnLeave(Starfish& star) {
}

void StarfishAttackState::DoAttack(Starfish& star) {
  if (star.GetField()->GetAt(star.GetTile()->GetX() - 1, star.GetTile()->GetY())) {
    Spell* spell = new Bubble(star.GetField(), star.GetTeam(), (star.GetRank() == Starfish::Rank::SP) ? 1.5 : 1.0);
    spell->SetHitboxProperties({ 40, static_cast<Hit::Flags>(spell->GetHitboxProperties().flags | Hit::impact), Element::AQUA, &star });
    spell->SetDirection(Direction::LEFT);
    star.GetField()->AddEntity(*spell, star.GetTile()->GetX() - 1, star.GetTile()->GetY());
  }
  
  if (--bubbleCount == 0) {
	star.GetFirstComponent<AnimationComponent>()->SetPlaybackMode(Animator::Mode::NoEffect);
	star.GetFirstComponent<AnimationComponent>()->CancelCallbacks();
	
	// On animation end, go back to idle
	star.GetFirstComponent<AnimationComponent>()->AddCallback(5, [this, s = &star](){
		s->ChangeState<StarfishIdleState>();
	}, Animator::NoCallback, false);
  }
}
