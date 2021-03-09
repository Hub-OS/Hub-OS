#pragma once
#include "frame_time_t.h"
#include <vector>
#include <queue>
#include <variant>
#include <any>

class CardAction;

namespace Battle {
  class Tile;
}

enum class ActionPriority : short {
  immediate = 0,
  card_chain = 1, // !< For cards used in successive animations under special conditions
  trigger = 2,
  forced,
  voluntary
};

struct MoveEvent {
  frame_time_t deltaFrames{}; //!< Frames between tile A and B. If 0, teleport. Else, we could be sliding
  frame_time_t delayFrames{}; //!< Startup lag to be used with animations
  frame_time_t endlagFrames{}; //!< Wait period before action is complete
  float height{}; //!< If this is non-zero with delta frames, the character will effectively jump
  Battle::Tile* dest{ nullptr };

  bool IsJumping() const {
    return dest && height > 0.f && deltaFrames > frames(0);
  }

  bool IsSliding() const {
    return dest && deltaFrames > frames(0) && height <= 0.0f;
  }

  bool IsTeleporting() const {
    return dest && deltaFrames == frames(0) && (+height) == 0.0f;
  }
};

struct BusterEvent {
  frame_time_t deltaFrames{}; //!< e.g. how long it animates
  frame_time_t endlagFrames{}; //!< Wait period after completition

  // Default is false which is shoot-then-move
  bool blocking{}; //!< If true, blocks incoming move events for auto-fire behavior
};

struct ActionEvent {
  ActionPriority value{};
  std::variant<MoveEvent, BusterEvent, CardAction*> data;
};

// TODO: create bnActionQueue.cpp and put function impl there so I can include CardAction headers...
struct ActionComparitor {
  bool operator()(const ActionEvent& a, const ActionEvent& b) {
    if (a.value < b.value) {
      return true;
    }
    
    // else 
    auto visitor = [](const ActionEvent& event) -> short {
      short p = std::numeric_limits<short>::max();

      auto visitBusterEvent = [&p](const BusterEvent&) {
        p = 1;
      };

      auto visitCardEvent = [&p](const CardAction* action) {
        p = 2;
      };

      auto visitMoveEvent = [&p](const MoveEvent&) {
        p = 3;
      };

      std::visit(overload(visitBusterEvent, visitCardEvent, visitMoveEvent), event.data);

      return p;
    };

    return visitor(a) < visitor(b);
  }
};

// overload design pattern for variant visits
template <class... Fs>
struct overload : Fs... {
  overload(const Fs&... fs) : Fs{ fs }...
  {}

  using Fs::operator()...;
};

template <class... Ts>
overload(Ts&&...)->overload<std::remove_reference_t<Ts>...>;

using ActionQueue = std::priority_queue<ActionEvent, std::vector<ActionEvent>, ActionComparitor>;
