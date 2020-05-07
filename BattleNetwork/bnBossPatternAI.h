#pragma once
#include "bnAIState.h"
#include "bnEntity.h"
#include "bnAgent.h"
#include "bnNoState.h"

template<typename CharacterT>
class BossPatternAI : public Agent {
private:
  std::vector<AIState<CharacterT>*> stateMachine; /*!< State machine responsible for state management */
  AIState<CharacterT>* interruptState;
  int stateIndex;
  CharacterT* ref; /*!< AI of this instance */
  int lock; /*!< Whether or not a state is locked */
  bool isUpdating, gotoNext, beginInterrupt, endInterrupt;
protected:
  /**
  * @brief State machine can or cannot be changed */
  enum StateLock {
    Locked,
    Unlocked
  };

public:
  // Used for SFINAE events that require characters with AI
  using IsUsingAI = CharacterT;

  /**
   * @brief Prevents the AI state to be changed. Must be unlocked to use again.
   */
  void LockState() {
    lock = BossPatternAI<CharacterT>::StateLock::Locked;
  }

  /**
   * @brief Allows the AI state to be changed.
   */
  void UnlockState() {
    lock = BossPatternAI<CharacterT>::StateLock::Unlocked;
  }

  /**
   * @brief Construct an AI with the object ref
   * @param _ref object to pass around the state
   */
  BossPatternAI(CharacterT* _ref) : Agent() {
    interruptState = nullptr;
    ref = _ref;
    lock = BossPatternAI<CharacterT>::StateLock::Unlocked;
    isUpdating = gotoNext = beginInterrupt = endInterrupt = false;
    stateIndex = 0;
  }

  /**
   * @brief Deletes the state machine object and Frees target
   */
  ~BossPatternAI() { 
    for (auto states : stateMachine) {
      delete states;
    }

    stateMachine.clear();

    ref = nullptr; FreeTarget(); 
  }

  void InvokeDefaultState() {
    if (interruptState) {
      endInterrupt = true;

      // Interrupt behavior increases the counter
      // So this forces next step to reset to index 0
      stateIndex = (int)stateMachine.size(); 
      return;
    }

    stateIndex = 0;
  }

  void GoToNextState() {
    gotoNext = true;

    if (interruptState) { endInterrupt = true; }
  }

  /**
   * @brief Change to state U. No arguments.
   */
  template<typename U>
  void AddState() {
    if (lock == BossPatternAI<CharacterT>::StateLock::Locked) {
      return;
    }

    stateMachine.push_back(new U());
  }

  /**
   * @brief For states that require arguments, pass the arguments
   *
   * e.g. AddState<PlayerThrowBombState>(200.f, 300, true);
   */
  template<typename U, typename ...Args>
  void AddState(Args... args) {
    if (lock == BossPatternAI<CharacterT>::StateLock::Locked) {
      return;
    }

    stateMachine.push_back(new U(std::forward<Args>(args)...));
  }

  /**
 * @brief Change to state U. No arguments.
 */
  template<typename U>
  void InterruptState() {
    if (lock == BossPatternAI<CharacterT>::StateLock::Locked) {
      return;
    }

    if (interruptState) { 
      interruptState->OnLeave(*ref);
      delete interruptState;  
    }

    interruptState = new U();
    beginInterrupt = true;
  }

  /**
   * @brief For states that require arguments, pass the arguments
   *
   * e.g. AddState<PlayerThrowBombState>(200.f, 300, true);
   */
  template<typename U, typename ...Args>
  void InterruptState(Args... args) {
    if (lock == BossPatternAI<CharacterT>::StateLock::Locked) {
      return;
    }

    if (interruptState) { 
      interruptState->OnLeave(*ref);
      delete interruptState; 
    }

    interruptState = new U(std::forward<Args>(args)...);
    beginInterrupt = true;
  }

  /**
   * @brief Update the SM
   * @param _elapsed in seconds
   *
   * If a change state request is made inside of a state, change to that state at end of update
   */
  void Update(float _elapsed) {
    if (isUpdating) return; // Prevent crashes from updating inside an active state

    isUpdating = true;

    if (interruptState) {
      if (beginInterrupt) {
        stateMachine[stateIndex]->OnLeave(*ref);
        interruptState->OnEnter(*ref);
        beginInterrupt = false;
      }

      interruptState->Update(_elapsed, *ref);

      if (endInterrupt) {
        interruptState->OnLeave(*ref);

        stateIndex++;

        if (stateIndex >= stateMachine.size()) {
          stateIndex = 0;
        }

        //auto old = stateMachine[stateIndex];
        //stateMachine[stateIndex];
        //delete old;

        stateMachine[stateIndex]->OnEnter(*ref);

        endInterrupt = false;

        delete interruptState;
        interruptState = nullptr;
      }
    } else if (stateIndex < stateMachine.size()) {
      stateMachine[stateIndex]->Update(_elapsed, *ref);
    }

    if (gotoNext) {
      stateMachine[stateIndex]->OnLeave(*ref);

      stateIndex++;

      if (stateIndex >= stateMachine.size()) {
        stateIndex = 0;
      }

      gotoNext = false;

      stateMachine[stateIndex]->OnEnter(*ref);
    }

    isUpdating = false;
  }
};