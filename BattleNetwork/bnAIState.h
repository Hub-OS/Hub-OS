#pragma once

#include "bnEntity.h"

// forward decl
template<typename T>
class AI;

/**
 * @class AIState
 * @author mav
 * @date 13/05/19
 * @brief The state of an AI. AI logic goes here.
 * 
 * To prevent UB, delays a state change by storing it in an object.
 * If an AI is changed to a state outside of this class, that state takes priority.
 */
template<class T>
class AIState
{
  friend class AI<T>;

private:
  AIState<T>* nextState; /*!< Delayed state change object */

  /**
   * @brief Transfers ownership to the state machine 
   * @see bnAI.h
   * @return nullptr if no next state, otherwise AIState<T>*
   */
  AIState<T>* GetNextState() {
    AIState<T>* out = nextState;
    nextState = nullptr;
    return out;
  }


public:
  /**
   * @brief Sets nextState to nullptr
   */
  AIState() { nextState = nullptr; }
  AIState(const AIState<T>& rhs) = default;
  AIState(AIState<T>&& ref) = default;

  /**
   * @brief Same syntax as AI<T>, constructs a delayed state change
   */
  template<class U, typename ...Args>
  void ChangeState(Args... args) {
    if (nextState) { delete nextState; }

    nextState = new U(args...);
  }

  /**
   * @brief Same syntax as AI<T>, constructs a delayed state change
   */
  template<class U>
  void ChangeState() {
    if (nextState) { delete nextState; }
        nextState = new U();
  }

  /**
   * @brief Updates the state and returns a new state if it was requested
   * @param _elapsed in seconds
   * @param context the referenced AI object
   * @return nextState
   */
  AIState<T>* Update(float _elapsed, T& context) {
    // nextState could be non-null after update
    OnUpdate(_elapsed, context);

    return nextState;
  }

  virtual ~AIState() { /*if (nextState) { delete nextState;  }*/ }

  /**
   * @brief Initialize the reference object when this state is created
   * @param context reference object
   */
  virtual void OnEnter(T& context) = 0;
  
  /**
   * @brief Update the reference object
   * @param _elapsed in seconds
   * @param context reference object
   */
  virtual void OnUpdate(float _elapsed, T& context) = 0;
  
  /**
   * @brief Prepare the reference object for the next state, when state changes
   * @param context reference object
   */
  virtual void OnLeave(T& context) = 0;
};

