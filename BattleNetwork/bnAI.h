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
  AIState<CharacterT>* stateMachine{ nullptr }; /*!< State machine responsible for state management */
  AIState<CharacterT>* queuedState{ nullptr }; /*!< State due to change to in the next update */
  CharacterT* ref{ nullptr }; /*!< AI of this instance */
  bool isUpdating{ false }; /*!< Safely ignore any extra Update() requests */
  int priorityLevel{std::numeric_limits<int>::max()};
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
    priorityLevel = 999;
  }
  
  /**
   * @brief Deletes the state machine object and Frees target
   */
  ~AI() {
    if (stateMachine) {
      delete stateMachine;
    }

    if (queuedState) {
      delete queuedState;
    }

    ref = nullptr;
    FreeTarget();
  }

  void InvokeDefaultState() {
    using DefaultState = typename CharacterT::DefaultState;

    ChangeState<DefaultState>();
  }

  /**
   * @brief Change to state U. No arguments.
   */
  template<typename U>
  void ChangeState() {
    bool change = true;

    if (stateMachine && stateMachine->locked) {
      if (priorityLevel > U::PriorityLevel) {
        stateMachine->PriorityUnlock();
      }
      else {
        change = false;
      }
    }
    else if (queuedState && queuedState->locked) {
      if (priorityLevel > U::PriorityLevel) {
        queuedState->PriorityUnlock();
      }
      else {
        change = false;
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
 * e.g. ChangeState<PlayerThrowBombState>(200.f, 300, true);
 */
template<typename U, typename ...Args>
void ChangeState(Args&&... args) {
  bool change = true;

  if (stateMachine && stateMachine->locked) {
    if (priorityLevel > U::PriorityLevel) {
      stateMachine->PriorityUnlock();
    }
    else {
      change = false;
    }
  }
  else if (queuedState && queuedState->locked) {
    if (priorityLevel > U::PriorityLevel) {
      queuedState->PriorityUnlock();
    }
    else {
      change = false;
    }
  }

  if (change) {
    if (queuedState) { delete queuedState; }
    queuedState = new U(std::forward<Args>(args)...);

    priorityLevel = U::PriorityLevel;
  }
}

template<typename U, typename Tuple, size_t... Is>
void ChangeState(const Tuple& tupleArgs, std::index_sequence<Is...>) {
  ChangeState<U>(std::get<Is>(tupleArgs)...);
}

template<typename U, typename Tuple, size_t Sz>
void ChangeState(const Tuple& tupleArgs) {
  ChangeState<U>(tupleArgs, std::make_index_sequence<Sz>());
}

/**
 * @brief Update the SM
 * @param _elapsed in seconds
 * 
 * If a change state request is made inside of a state, change to that state at end of update
 */
  void Update(double _elapsed) {
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