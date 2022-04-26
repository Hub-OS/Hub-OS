#pragma once
#include "bnInputHandle.h"
#include "bnInputManager.h"
#include <functional>

class InputRepeater : public InputHandle {
private:
  double inputCooldown{}, maxInputCooldown{ 0.25 }, delayFactor{ 0.25 };
  bool extendedHold{}, executeThisFrame{}, triggered{};
public:
  bool HandleInput(const std::initializer_list<InputEvent>& events, const std::function<void()> callback);
  bool HandleInput(const InputEvent& event, const std::function<void()> callback);
  void Update(double elapsed);
  void Reset();
};