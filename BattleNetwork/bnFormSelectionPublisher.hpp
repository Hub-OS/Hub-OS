#pragma once

#include <vector>

class FormSelectionListener;
/**
 * @brief Emits an event when the player changes their form selection. Primarily used when the player is in the chip
 * selection screen.
 */
class FormSelectionPublisher {
  friend class FormSelectionListener;
public:
  FormSelectionPublisher();
  virtual ~FormSelectionPublisher();

  /**
   * @brief Notifies listeners that the player has selected a form.
   * @param index Index of selected player form, -1 if no form;
   */
  void Broadcast(int index);

private:
  std::vector<FormSelectionListener*> listeners;

  void AddListener(FormSelectionListener* listener) {
    listeners.push_back(listener);
  }
};
