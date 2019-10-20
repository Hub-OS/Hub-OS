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
  double progress;
  bool prevFloatShoe;

public:
  inline static const int PriorityLevel = 1;

  BubbleState();
  virtual ~BubbleState();

  void OnEnter(Any& e);
  void OnUpdate(float _elapsed, Any& e);
  void OnLeave(Any& e);
};

#include "bnField.h"
#include "bnLogger.h"

template<typename Any>
BubbleState<Any>::BubbleState()
  : progress(0), AIState<Any>() {
}

template<typename Any>
BubbleState<Any>::~BubbleState() {
  /* artifact is deleted by field */
}

template<typename Any>
void BubbleState<Any>::OnEnter(Any& e) {
  prevFloatShoe = e.HasFloatShoe();
  e.SetFloatShoe(true);
  e.PriorityLock();
}

template<typename Any>
void BubbleState<Any>::OnUpdate(float _elapsed, Any& e) {
  // Check if bubbletrap is removed from entity
  if (e.template GetFirstComponent<BubbleTrap>() == nullptr) {
    e.PriorityUnlock();
    e.ChangeState<Any::DefaultState>();
    e.SetFloatShoe(prevFloatShoe);
  }

  sf::Vector2f offset = sf::Vector2f(0, 5.0f + 10.0f * std::sin((float)progress * 10.0f));
  e.setPosition(e.getPosition() - offset);
  e.SetAnimation("PLAYER_HIT"); // playing over and over from the start creates a freeze frame effect

  progress += _elapsed;
}

template<typename Any>
void BubbleState<Any>::OnLeave(Any& e) {
  //std::cout << "left bubblestate" << std::endl;

  AUDIO.Play(AudioType::BUBBLE_POP);
}
