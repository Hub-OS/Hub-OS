#pragma once

class FormSelectionPublisher;
/**
 * @brief Allows unrelated types to communicate when the user has selected or de-selected a form.
 *
 * The player selects a form before Megaman (/other navi) actually transforms.
 */
class FormSelectionListener {
public:
  FormSelectionListener();
  virtual ~FormSelectionListener();
  FormSelectionListener(const FormSelectionListener&) = delete;
  FormSelectionListener(FormSelectionListener&&) = delete;

  /**
   * @brief Notification function called when the player selects a form.
   * @param index Index of selected player form, -1 if no form;
   */
  virtual void OnFormSelected(int index) = 0;

  /**
   * @brief Subscribe to a potential publisher
   * @param publisher Source of a form selection event
   */
  void Subscribe(FormSelectionPublisher& publisher);
};
