#pragma once

#include "bnFont.h"
#include <SFML/Window/Event.hpp>
#include <cfloat>

class InputManager;

/**
 * @class InputTextBuffer
 * @date 06/5/21
 * @brief Handles text input including cursor support
 */
class InputTextBuffer {
public:
  InputTextBuffer() { Reset(); };

  // no copies
  InputTextBuffer(const InputTextBuffer&) = delete;

  /**
   * @brief Enables text capture (disabled by default)
   */
  void BeginCapture();

  /**
   * @brief Disables text capture (disabled by default)
   */
  void EndCapture();

  /**
   * @brief Calculate line indexes for resolving caret position. Call this after setting text and configuring.
   */
  void CalculateLineIndexes();

  /**
   * @brief Resets all settings and clears the captured text
   */
  void Reset();

  /**
   * @brief Returns true if the text was modified since SetText/ToString
   * @return bool
   */
  bool IsModified();

  /**
   * @brief Returns the contents of the current captured text
   * @return const std::string
   */
  const std::string& ToString();

  /**
   * @brief Overloads the captured text
   * @param buff text to modify
   *
   * Used when selecting existing input fields
   * After being set, the buffer can be deleted with backspace or modified by typing
   */
  void SetText(const std::string& buff);

  /**
   * @brief Inserts a character at the caret position and moves the caret
   * @param c character to insert
   */
  void InsertCharacter(char c);

  /**
   * @brief Returns a row column pair of the caret
   * @return std::pair<size_t, size_t> { row, col }
   */
  std::pair<size_t, size_t> GetRowCol();

  /**
   * @brief Get the insertion point of new characters
   * @param
   */
  const size_t GetCaretPosition() const;

  /**
   * @brief Set the insertion point of new characters
   * @param
   */
  void SetCaretPosition(size_t pos);

  /**
   * @brief Moves the caret to the relative cursor position
   * @param
   */
  void MoveCaretToCursor(const sf::Vector2f& cursor);

  /**
   * @brief
   * @param
   */
  void SetFont(const Font& font);

  /**
   * @brief
   * @param
   */
  void SetLineWidth(float width);

  /**
   * @brief Set the max size of the buffer
   * @param
   */
  void SetCharacterLimit(size_t limit);

  void SetIgnoreNewLine(bool ignoreNewLine);

  void ProtectPassword(bool isPassword);

private:
  bool captureInput{};
  std::vector<size_t> lineIndexes;
  std::string buffer;
  size_t caretPos{};
  Font font{ Font::Style::thin };
  float maxLineWidth{ FLT_MAX };
  size_t characterLimit{ SIZE_MAX };
  bool ignoreNewLine{};
  bool modified{};
  bool modifiedThisRun{};
  bool carriageReturn{};
  bool password{};

  /**
   * @brief Transforms SFML keycodes into ASCII char texts and stores into input buffer
   * @param e expects sf::Event::TextEntered
   */
  void HandleTextEntered(sf::Event e);

  /**
   * @brief Transforms SFML keycodes into caret movements
   * @param e expects sf::Event::KeyPressed
   */
  void HandleKeyPressed(sf::Event e);

  void HandlePaste(const std::string& data);

  void HandleCompletedEventProcessing();

  void LineUp();
  void LineDown();

  void MarkModified();

  friend class InputManager;
};
