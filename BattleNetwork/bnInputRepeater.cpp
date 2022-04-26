#include "bnInputRepeater.h"

void InputRepeater::Reset() {
  inputCooldown = 0;
  extendedHold = false;
  executeThisFrame = false;
  triggered = false;
}

bool InputRepeater::HandleInput(const std::initializer_list<InputEvent>& events, const std::function<void()> callback)
{
  bool tryExecute = false;

  for (const InputEvent& e : events) {
    tryExecute = Input().Has(e) || tryExecute;

    if (tryExecute) break;
  }

  if(tryExecute) {
    triggered = true;

    if (executeThisFrame) {
      callback();
    }
    return true;
  }

  return false;
}

bool InputRepeater::HandleInput(const InputEvent& event, const std::function<void()> callback)
{
  return HandleInput({ event }, callback);
}

void InputRepeater::Update(double elapsed) {
  if (triggered) {
    inputCooldown -= elapsed;
  }

  triggered = false;
  executeThisFrame = false;

  if (inputCooldown <= 0) {
    if (!extendedHold) {
      inputCooldown = maxInputCooldown;
      extendedHold = true;
    }
    else {
      inputCooldown = maxInputCooldown * delayFactor;
    }

    executeThisFrame = true;
  }
}