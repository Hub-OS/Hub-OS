#pragma once
#include <list>
#include <functional>

/**
 * @class DefenseFrameStateArbiter
 * @author mav
 * @date 05/05/20
 * @brief This class is used to help determine the correct outcome for combat
 *
 * This arbiter is passed into every defense rule check to correctly determine
 * if a rule should block and if it triggers. If a defense rule has a trigger (callback),
 * then it is added to the list. It might be that most fail upon the final combat step.
 * In that case, the triggers will be cleared and unused.
 */
class DefenseFrameStateArbiter final {
  bool blockedDamage, blockedImpact;
  std::list<std::function<void()>> triggers;

public:
  DefenseFrameStateArbiter() = default;
  DefenseFrameStateArbiter(const DefenseFrameStateArbiter&) = delete;
  ~DefenseFrameStateArbiter() = default;

  const bool IsDamageBlocked() const;
  const bool IsImpactBlocked() const;
  void BlockDamage();
  void BlockImpact();

  template<typename Func, typename... Args>
  void AddTrigger(const Func& func, Args&&... args);

  void ExecuteAllTriggers();
};

template<typename Func, typename... Args>
void DefenseFrameStateArbiter::AddTrigger(const Func& func, Args&&... args) {
  auto closure = [func, args...]() -> void {
      std::invoke(func, args...);
  };

  triggers.push_back(closure);
}