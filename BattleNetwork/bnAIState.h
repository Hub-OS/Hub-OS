#pragma once

#include "bnEntity.h"

// forward decl
template<typename T>
class AI;

// forward decl
template<typename T>
class BossPatternAI;

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
  bool locked{ false };

public:
  friend class AI<T>;
  friend class BossPatternAI<T>;

  /**
   * @brief Ctor
   */
  AIState() = default;
  AIState(const AIState<T>& rhs) = default;
  AIState(AIState<T>&& ref) = default;

  // Default
  inline static const int PriorityLevel = 999;

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

  /**
  * @brief Locks state so other states cannot change it unless the priority of the new state is higher
  * 
  * Sets `locked` to true
  */
  void PriorityLock() {
    locked = true;
  }

  /**
  * @brief Unlocks state so other states can change freely
  *
  * Sets `locked` to false
  */
  void PriorityUnlock() {
    locked = false;
  }
};

