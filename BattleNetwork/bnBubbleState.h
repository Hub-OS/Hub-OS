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
 * It uses constraints to ensure the type passed in Any
 * is a subclass of Entity.
 */
template<typename Any, typename NextState>
class BubbleState : public AIState<Any>
{
protected:
  double progress;
  //BubbleTrap* trap;

public:
  BubbleState();
  virtual ~BubbleState();

  void OnEnter(Any& e);
  void OnUpdate(float _elapsed, Any& e);
  void OnLeave(Any& e);
};

#include "bnField.h"
#include "bnLogger.h"

template<typename Any, typename NextState>
BubbleState<Any, NextState>::BubbleState()
  : progress(0), AIState<Any>() {
}

template<typename Any, typename NextState>
BubbleState<Any, NextState>::~BubbleState() {
  /* artifact is deleted by field */
}

template<typename Any, typename NextState>
void BubbleState<Any, NextState>::OnEnter(Any& e) {
  e.LockState(); // Lock AI state. We cannot be forced out of this.
}

template<typename Any, typename NextState>
void BubbleState<Any, NextState>::OnUpdate(float _elapsed, Any& e) {
  // Check if bubbletrap is removed from entity
  if (e.template GetComponent<BubbleTrap>() == nullptr) {
    e.UnlockState();
    this->template ChangeState<NextState>();
  }

  sf::Vector2f offset = sf::Vector2f(0, 5.0f + 10.0f * std::sin((float)progress * 10.0f));
  e.setPosition(e.getPosition() - offset);
  e.SetAnimation("PLAYER_HIT"); // playing over and over from the start creates a freeze frame effect

  progress += _elapsed;
}

template<typename Any, typename NextState>
void BubbleState<Any, NextState>::OnLeave(Any& e) { 
  //std::cout << "left bubblestate" << std::endl;

  AUDIO.Play(AudioType::BUBBLE_POP);
}
