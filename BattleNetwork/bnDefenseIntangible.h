#pragma once
#include "frame_time_t.h"
#include "bnDefenseRule.h"
#include <vector>
#include <functional>

class DefenseIntangible : public DefenseRule {
public:
  DefenseIntangible();
  ~DefenseIntangible();

  void Update();
  bool IsEnabled();
  void Enable(
    frame_time_t frames,
    bool retangibleWhenPierced,
    Hit::Flags hitWeaknesses,
    std::vector<Element> elemWeaknesses,
    std::function<void()> onDeactivate
  );
  void Disable();
  bool Pierces(const Hit::Properties& statuses);
  bool TryPierce(const Hit::Properties& statuses);
  bool IsRetangible();

  Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) override;

private:
  frame_time_t cooldown{};
  bool retangibleWhenPierced{};
  Hit::Flags hitWeaknesses{};
  std::vector<Element> elemWeaknesses;
  std::function<void()> onDeactivate;
  bool retangible;
};
