#pragma once
#include "bnMeta.h"
#include "bnAIState.h"
#include "bnEntity.h"
#include "bnAgent.h"
#include "bnNoState.h"

/**
 * @class AI
 * @author mav
 * @date 13/05/19
 * @file bnAI.h
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
template<typename T>
class AI : public Agent {
private:
  AIState<T>* stateMachine; /*!< State machine responsible for state management */
  T* ref; /*!< AI of this instance */
  int lock; /*!< Whether or not a state is locked */

protected:
  /**
  * @brief State machine can or cannot be changed */
  enum StateLock {
    Locked,
    Unlocked
   };

public:

  /**
   * @brief Prevents the AI state to be changed. Must be unlocked to use again.
   */
  void LockState() {
    lock = AI<T>::StateLock::Locked;
  }

  /**
   * @brief Allows the AI state to be changed.
   */
  void UnlockState() {
    lock = AI<T>::StateLock::Unlocked;
  }
 
  /**
   * @brief Construct an AI with the object ref
   * @param _ref object to pass around the state
   */
  AI(T* _ref) : Agent() { stateMachine = nullptr; ref = _ref; lock = AI<T>::StateLock::Unlocked; }
  
  /**
   * @brief Deletes the state machine object and Frees target
   */
  ~AI() { if (stateMachine) { delete stateMachine; } ref = nullptr; this->FreeTarget(); }

  /**
   * @brief Change to state U. No arguments.
   */
  template<typename U>
  void ChangeState() {
    if (lock == AI<T>::StateLock::Locked) {
      return;
    }

    // For easy checks below, provide a null state to leave from
    if (!stateMachine) {
      stateMachine = new NoState<T>();
    }

    // Change to U
    stateMachine->template ChangeState<U>();

    // Get the U state object created by the above template call
    AIState<T>* nextState = stateMachine->GetNextState();

    //TODO: verify, this should always be non null...
    if (nextState != nullptr) {
      // Leave last state
      stateMachine->OnLeave(*ref);

      // Set the next state and preserve the ptr of old
      AIState<T>* oldState = stateMachine;
      stateMachine = nextState;
      stateMachine->OnEnter(*ref);
      
      // Cleanup old
      delete oldState;
      oldState = nullptr;
    }
  }

/**
 * @brief For states that require arguments, pass the arguments
 * 
 * e.g. this->ChangeState<PlayerThrowBombState>(200.f, 300, true);
 */
template<typename U, typename ...Args>
  void ChangeState(Args... args) {
    if (lock == AI<T>::StateLock::Locked) {
      return;
    }

    if (!stateMachine) {
      stateMachine = new NoState<T>();
    }

    stateMachine->template ChangeState<U>(args...);

    AIState<T>* nextState = stateMachine->GetNextState();

    if (nextState != nullptr) {

      stateMachine->OnLeave(*ref);

      AIState<T>* oldState = stateMachine;
      stateMachine = nextState;
      stateMachine->OnEnter(*ref);
      delete oldState;
      oldState = nullptr;
    }
  }

/**
 * @brief Update the SM
 * @param _elapsed in seconds
 * 
 * If a change state request is made inside of a state, change to that state at end of update
 */
  void Update(float _elapsed) {
    if (stateMachine != nullptr) {
      AIState<T>* nextState = stateMachine->Update(_elapsed, *ref);

      if (nextState != nullptr) {

        stateMachine->OnLeave(*ref);

        AIState<T>* oldState = stateMachine;
        stateMachine = nextState;
        stateMachine->OnEnter(*ref);
        delete oldState;
        oldState = nullptr;
      }
    }
  }
};
