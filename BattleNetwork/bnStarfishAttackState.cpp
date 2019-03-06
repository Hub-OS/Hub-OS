#include "bnStarfishIdleState.h"
#include "bnBubble.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnStarfishAttackState.h"

StarfishAttackState::StarfishAttackState(int maxBubbleCount) : bubbleCount(maxBubbleCount), AIState<Starfish>() { ; }
StarfishAttackState::~StarfishAttackState() { ; }

void StarfishAttackState::OnEnter(Starfish& star) {
  auto onPreAttack = [this, s = &star]() { 

    auto onAttack = [this, s]() {
      this->DoAttack(*s); 
    };

    s->animationComponent.SetAnimation("ATTACK", Animate::Mode::Loop);
    s->OnFrameCallback(10, onAttack, std::function<void()>(), false);
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
  if (star.GetField()->GetAt(star.GetTile()->GetX() - 1, star.GetTile()->GetY())) {
    Spell* spell = new Bubble(star.field, star.team, (star.GetRank() == Starfish::Rank::SP) ? 1.5 : 1.0);
    spell->SetHitboxProperties({ 40, static_cast<Hit::Flags>(spell->GetHitboxProperties().flags | Hit::impact), Element::AQUA, 3.0, &star });
    spell->SetDirection(Direction::LEFT);
    star.field->AddEntity(*spell, star.tile->GetX() - 1, star.tile->GetY());
  }
  
  //std::cout << "bubble count: " << bubbleCount << std::endl;

  if (--bubbleCount == 0) {
	star.animationComponent.CancelCallbacks();
	
	// On animation end, go back to idle
	star.animationComponent.AddCallback(10, [this]() {
		this->ChangeState<StarfishIdleState>();
	}, std::function<void()>());
  }
}
