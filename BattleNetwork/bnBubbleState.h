#pragma once

#include "bnEntity.h"
#include "bnAIState.h"
#include "bnBubbleTrap.h"
#include "bnShaderResourceManager.h"
#include <cmath>

/**
 * @class BubbleState
 * @author mav
 * @date 05/05/19
 * @brief This state traps an entity in a bubble for a duration of time
 * 
 * This state can be used by any Entity in the engine.
 */
template<typename Any>
class BubbleState : public AIState<Any>
{
protected:
  frame_time_t progress;
  bool prevFloatShoe;

public:
  inline static const int PriorityLevel = 1;

  BubbleState();
  virtual ~BubbleState();

  void OnEnter(Any& e) override;
  void OnUpdate(double _elapsed, Any& e) override;
  void OnLeave(Any& e) override;
};

#include "bnField.h"
#include "bnLogger.h"
#include "bnAudioResourceManager.h"

template<typename Any>
BubbleState<Any>::BubbleState()
  : progress(frames(0)), prevFloatShoe(false), AIState<Any>() {
  this->PriorityLock();
}

template<typename Any>
BubbleState<Any>::~BubbleState() {
  /* artifact is deleted by field */
}

template<typename Any>
void BubbleState<Any>::OnEnter(Any& e) {
  prevFloatShoe = e.HasFloatShoe(); // Hack: bubble would be otherwise pushed by moving tiles
  e.SetFloatShoe(true);
  auto animationComponent = e.template GetFirstComponent<AnimationComponent>();
  if (animationComponent) { animationComponent->CancelCallbacks(); }
  e.ClearActionQueue();
  e.FinishMove();
}

template<typename Any>
void BubbleState<Any>::OnUpdate(double _elapsed, Any& e) {
  auto bubbleTrap = e.template GetFirstComponent<BubbleTrap>();

  // Check if bubbletrap is removed from entity
  if (bubbleTrap == nullptr) {
    e.template ChangeState<typename Any::DefaultState>();
    e.SetFloatShoe(prevFloatShoe);
    this->PriorityUnlock();
  }
  else {
    progress = bubbleTrap->GetDuration();

    sf::Vector2f offset = sf::Vector2f(0, 5.0f + 10.0f * std::sin((float)progress.asSeconds().value * 10.0f));
    e.SetDrawOffset(-offset);
    e.SetAnimation("PLAYER_HIT"); // playing over and over from the start creates a freeze frame effect
  }
}

template<typename Any>
void BubbleState<Any>::OnLeave(Any& e) {
  //std::cout << "left bubblestate" << std::endl;
  e.SetDrawOffset(sf::Vector2f{});
  ResourceHandle().Audio().Play(AudioType::BUBBLE_POP);
}
