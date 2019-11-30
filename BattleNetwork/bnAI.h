#pragma once
#include "bnAIState.h"
#include "bnEntity.h"
#include "bnAgent.h"
#include "bnNoState.h"
/**
 * @class AI
 * @author mav
 * @date 13/05/19
 * @brief AI is a state machine for Agents
 * 
 * AI uses state machines to change states to appear intelligent. 
 * This strategy makes the code base easier to work with and removes code from
 * the Entity's source code.
 * 
 * The SM uses a delayed state change so as not to cause undefined behavior.
 * 
 * @warning It is not safe to call Update() in any AI state
 */
template<typename CharacterT>
class AI : public Agent {
private:
  AIState<CharacterT>* stateMachine; /*!< State machine responsible for state management */
  CharacterT* ref; /*!< AI of this instance */
  bool isUpdating; /*!< Safely ignore any extra Update() requests */
  AIState<CharacterT>* queuedState;
  int priorityLevel; 
  bool priorityLocked;
public:
  // Used for SFINAE events that require characters with AI
  using IsUsingAI = CharacterT;
 
  /**
   * @brief Construct an AI with the object ref
   * @param _ref object to pass around the state
   */
  AI(CharacterT* _ref) : Agent() { 
    stateMachine = queuedState = nullptr; 
    ref = _ref;
    isUpdating = false;
    priorityLocked = false;
    priorityLevel = 999;
  }
  
  /**
   * @brief Deletes the state machine object and Frees target
   */
  ~AI() { if (stateMachine) { delete stateMachine; } ref = nullptr; this->FreeTarget(); }

  void InvokeDefaultState() {
    using DefaultState = typename CharacterT::DefaultState;

    this->ChangeState<DefaultState>();
  }

  void PriorityLock() {
    priorityLocked = true;
  }

  void PriorityUnlock() {
    priorityLocked = false;
  }

  /**
   * @brief Change to state U. No arguments.
   */
  template<typename U>
  void ChangeState() {
    bool change = true;

    if (priorityLocked) {
      change = false;

      if (priorityLevel > U::PriorityLevel) {
        change = true;
        priorityLocked = false; // unlock
      }
    }

    if (change) {
      if (queuedState) { delete queuedState; }
      queuedState = new U();

      priorityLevel = U::PriorityLevel;
    }
  }

/**
 * @brief For states that require arguments, pass the arguments
 * 
 * e.g. this->ChangeState<PlayerThrowBombState>(200.f, 300, true);
 */
template<typename U, typename ...Args>
  void ChangeState(Args... args) {
    bool change = true;

    if (priorityLocked) {
      change = false;

      if (priorityLevel > U::PriorityLevel) {
        change = true;
        priorityLocked = false; // unlock
      }
    }

    if (change) {
      if (queuedState) { delete queuedState; }
      queuedState = new U(args...);

      priorityLevel = U::PriorityLevel;
    }
  }

/**
 * @brief Update the SM
 * @param _elapsed in seconds
 * 
 * If a change state request is made inside of a state, change to that state at end of update
 */
  void Update(float _elapsed) {
    if (isUpdating) return;

    isUpdating = true;

    if (stateMachine != nullptr) {
      stateMachine->Update(_elapsed, *ref);

      if (queuedState != nullptr) {
        stateMachine->OnLeave(*ref);

        AIState<CharacterT>* oldState = stateMachine;
        stateMachine = queuedState;
        stateMachine->OnEnter(*ref);
        delete oldState;
        queuedState = nullptr;
      }
    }
    else {
      if (queuedState != nullptr) {
        stateMachine = queuedState;
        stateMachine->OnEnter(*ref);
        queuedState = nullptr;
      }
    }

    isUpdating = false;
  }
};