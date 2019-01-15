#pragma once

#include "bnEntity.h"
#include "bnMeta.h"

template<class T>
class AIState
{
private:
  AIState<T>* nextState;

public:
  AIState() { nextState = nullptr; }
  AIState(const AIState<T>& rhs) = default;
  AIState(AIState<T>&& ref) = default;

  template<class U, typename ...Args>
  void ChangeState(Args... args) {
    //_DerivedFrom<U, AIState<T>>();

    if (nextState) { delete nextState; }

    nextState = new U(args...);
  }

  template<class U>
  void ChangeState() {
    //_DerivedFrom<U, AIState<T>>();

    if (nextState) { delete nextState; }

    nextState = new U();
  }

  AIState<T>* Update(float _elapsed, T& context) {
    // if (nextState) { nextState = nullptr; }

    // nextState could be non-null after update
    OnUpdate(_elapsed, context);

    return nextState;
  }

  virtual ~AIState() { ; }

  virtual void OnEnter(T& context) = 0;
  virtual void OnUpdate(float _elapsed, T& context) = 0;
  virtual void OnLeave(T& context) = 0;
};

