#pragma once
#include <map>
#include <set>
#include <functional>
#include <memory>

class DefenseRule; // forward decl

/**
 * @class DefenseFrameStateJudge
 * @author mav
 * @date 05/05/20
 * @brief This class is used to help determine the correct outcome for combat
 *
 * This judge is passed into every defense rule check to correctly determine
 * if a rule should block and if it triggers. If a defense rule has a trigger (callback),
 * then it is added to the list. Each attack has its `blockedDamage` and `blockedImpact` states reset.
 * However, proceeding attacks could signal the DefnseWasPierced routine which disables the trigger.
 */
class DefenseFrameStateJudge final {
  bool blockedDamage, blockedImpact;
  std::map<std::shared_ptr<DefenseRule>, std::function<void()>> triggers;
  std::set<std::shared_ptr<DefenseRule>> piercedSet;
  std::shared_ptr<DefenseRule> context;
public:
  DefenseFrameStateJudge();
  DefenseFrameStateJudge(const DefenseFrameStateJudge&) = delete;
  ~DefenseFrameStateJudge() = default;

  const bool IsDamageBlocked() const;
  const bool IsImpactBlocked() const;
  void BlockDamage();
  void BlockImpact();
  void SignalDefenseWasPierced();
  void SetDefenseContext(std::shared_ptr<DefenseRule>);
  void PrepareForNextAttack();

  template<typename Func, typename... Args>
  void AddTrigger(const Func& func, Args&&... args);
  void ExecuteAllTriggers();
};

template<typename Func, typename... Args>
void DefenseFrameStateJudge::AddTrigger(const Func& func, Args&&... args) {
  if (!context) return;

  // Prevent pierced defenses from re-triggering
  for (auto pierced : piercedSet) {
    if (pierced == context) return;
  }

  // Create a closure around this function and its inputs...
  auto closure = [func, args...]() -> void {
      std::invoke(func, args...);
  };

  // Tag it with the defense rule that triggered it and store away in a unique map
  triggers.insert(std::make_pair(context,closure));
}