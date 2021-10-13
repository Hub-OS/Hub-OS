#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "bnInputEvent.h"

using std::vector;

class VirtualInputState {
private:
  std::unordered_map<std::string, InputState> state, stateLastFrame, queuedState;

public:
  void Process();
  const std::unordered_map<std::string, InputState> ToHash() const;
  /**
   * @brief Queries if an input event has been fired
   * @param _event the event to look for.
   * @return true if present, false otherwise
   */
  bool Has(InputEvent event) const;

  /**
   * @brief Checks if the input event list is empty
   * @return true if empty, false otherwise
   */
  bool Empty() const;

  /**
   * @brief fires a key press manually
   * @param event key event to fire
   *
   * Used best on android where the virtual touch pad areas must map to keys
   */
  void VirtualKeyEvent(InputEvent event);

  /**
  * @brief if any buttons are held or pressed, fire release events for all
  */
  void Flush();

  void DebugPrint();
};