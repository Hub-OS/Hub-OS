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

public:
  /**
   * @brief Ctor
   */
  AIState() { }
  AIState(const AIState<T>& rhs) = default;
  AIState(AIState<T>&& ref) = default;

 
  /**
   * @brief Updates the state
   * @param _elapsed in seconds
   * @param context the referenced AI object
   */
  void Update(float _elapsed, T& context) {
    OnUpdate(_elapsed, context);
  }

  virtual ~AIState() { }

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

