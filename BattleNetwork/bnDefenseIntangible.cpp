#include "bnDefenseIntangible.h"
#include <algorithm>

DefenseIntangible::DefenseIntangible() : DefenseRule(DefensePriority::Intangible, DefenseOrder::always) {}
DefenseIntangible::~DefenseIntangible() {}

bool DefenseIntangible::IsEnabled() {
  return cooldown > frames(0);
}

void DefenseIntangible::Update() {
  if (!IsEnabled()) {
    return;
  }

  cooldown -= frames(1);

  if (!IsEnabled()) {
    Disable();
  }
}

void DefenseIntangible::Enable(
  frame_time_t frames,
  bool retangibleWhenPierced,
  Hit::Flags hitWeaknesses,
  std::vector<Element> elemWeaknesses,
  std::function<void()> onDeactivate
) {
  if (IsEnabled()) {
    Disable();
  }

  cooldown = frames;
  this->retangibleWhenPierced = retangibleWhenPierced;
  this->hitWeaknesses = hitWeaknesses;
  this->elemWeaknesses = elemWeaknesses;
  this->onDeactivate = onDeactivate;
  retangible = false;
}

void DefenseIntangible::Disable() {
  cooldown = frames(0);
  auto tempOnDeactivate = onDeactivate;
  onDeactivate = nullptr;
  retangible = false;

  if (tempOnDeactivate) {
    tempOnDeactivate();
  }
}

bool DefenseIntangible::IsRetangible() {
  return retangible;
}

bool DefenseIntangible::Pierces(const Hit::Properties& statuses) {
  if ((statuses.flags & hitWeaknesses) != Hit::none) {
    // any hit weakness will make this test pass
    return true;
  }

  // test for any elem weakness
  return std::any_of(elemWeaknesses.begin(), elemWeaknesses.end(), [&statuses] (Element element) {
    return element == statuses.element || element == statuses.secondaryElement;
  });
}

bool DefenseIntangible::TryPierce(const Hit::Properties& statuses) {
  if (!Pierces(statuses)) {
    return false;
  }

  if (retangibleWhenPierced && (statuses.flags & Hit::retain_intangible) != Hit::none) {
    retangible = true;
  }

  return true;
}

Hit::Properties& DefenseIntangible::FilterStatuses(Hit::Properties& statuses) {
  return statuses;
}

void DefenseIntangible::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) {
  
}
