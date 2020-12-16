/*! \brief This state can be used by any Entity in the engine.
 * 
 * This state exists to keep the AIState non-null and
 * minimize run-time errors. This state exists as filler
 * for when no state is supplied for an AI. Not recommended
 * to be used in custom enemies.
*/

#pragma once
#include "bnEntity.h"
#include "bnAIState.h"

template<typename Any>
class NoState : public AIState<Any>
{
public:
  inline static const int PriorityLevel = 2;

  NoState();
  ~NoState();

  void OnEnter(Any& e);
  void OnUpdate(double _elapsed, Any& e);
  void OnLeave(Any& e);
};

template<typename Any>
NoState<Any>::NoState() : AIState<Any>() {

}

template<typename Any>
NoState<Any>::~NoState() { }

template<typename Any>
void NoState<Any>::OnEnter(Any& e) { }

template<typename Any>
void NoState<Any>::OnUpdate(double _elapsed, Any& e) { }

template<typename Any>
void NoState<Any>::OnLeave(Any& e) { }