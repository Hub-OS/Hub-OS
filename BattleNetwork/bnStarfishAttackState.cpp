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
  auto animation = star.GetFirstComponent<AnimationComponent>();

  auto onPreAttack = [this, s = &star, animation]() { 
    auto onAttack = [this, s, animation]() {
      DoAttack(*s); 
    };

    animation->SetAnimation("ATTACK", Animator::Mode::Loop);
    animation->AddCallback(1, onAttack);
  };

  animation->SetAnimation("PREATTACK", onPreAttack);
  animation->SetCounterFrameRange(1, 2);
}

void StarfishAttackState::OnUpdate(double _elapsed, Starfish& star) {
}

void StarfishAttackState::OnLeave(Starfish& star) {
}

void StarfishAttackState::DoAttack(Starfish& star) {
  auto animation = star.GetFirstComponent<AnimationComponent>();

  if (star.GetField()->GetAt(star.GetTile()->GetX() - 1, star.GetTile()->GetY())) {
    auto spell = std::make_shared<Bubble>(Team::unknown, (star.GetRank() == Starfish::Rank::SP) ? 1.5 : 1.0);
    spell->SetHitboxProperties({ 40, static_cast<Hit::Flags>(spell->GetHitboxProperties().flags | Hit::impact), Element::aqua, star.GetID() });
    spell->SetDirection(Direction::left);
    star.GetField()->AddEntity(spell, star.GetTile()->GetX() - 1, star.GetTile()->GetY());
  }
  
  if (--bubbleCount == 0) {
  animation->SetPlaybackMode(Animator::Mode::NoEffect);
  animation->CancelCallbacks();
  
  // On animation end, go back to idle
  animation->AddCallback(5, 
    [this, s = &star](){
      s->ChangeState<StarfishIdleState>();
    });
  }
}
