#pragma once
#include "bnMeta.h"
#include "bnEntity.h"
#include "bnAIState.h"
#include "bnBubbleTrap.h"
#include "bnShaderResourceManager.h"

/*
  This state can be used by any Entity in the engine.
  It uses constraints to ensure the type passed in Any
  is a subclass of Entity.

  This state traps an entity in a bubble for a duration of time
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
  // Enforce template constraints on class
  _DerivedFrom<Any, Entity>();
}

template<typename Any, typename NextState>
BubbleState<Any, NextState>::~BubbleState() {
  /* explosion artifact is deleted by field */
}

template<typename Any, typename NextState>
void BubbleState<Any, NextState>::OnEnter(Any& e) {
  std::cout << "entered bubblestate" << std::endl;

  e.LockState(); // Lock AI state. We cannot be forced out of this.
  //trap = new BubbleTrap(e);
  //e.GetField()->AddEntity(trap, e.GetTile().GetX(), e.GetTile().GetY());
}

template<typename Any, typename NextState>
void BubbleState<Any, NextState>::OnUpdate(float _elapsed, Any& e) {
  if (e.GetComponent<BubbleTrap>() == nullptr) {
    e.UnlockState();
    this->ChangeState<NextState>();
  }

  sf::Vector2f offset = sf::Vector2f(0, 5.0f + 10.0f * std::sinf((float)progress * 10.0f));
  e.setPosition(e.getPosition() - offset);
  e.SetAnimation("PLAYER_HIT"); // playing over and over from the start creates a freeze frame effect

  progress += _elapsed;
}

template<typename Any, typename NextState>
void BubbleState<Any, NextState>::OnLeave(Any& e) { 
  std::cout << "left bubblestate" << std::endl;

  //trap->Pop();
}