/*! \brief Define a rectangular area, font size, print speed, and a message that types out over time
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <cstdint>

#include "bnText.h"
#include "bnFont.h"
#include "bnResourceHandle.h"

class TextBox : public sf::Drawable, public sf::Transformable, public ResourceHandle {
public:
  typedef uint16_t vfx;

  struct effects {
    static const vfx none = 0x0000;
    static const vfx dramatic = 0x0001; // halves the text play-out animation for dramatic moments
    static const vfx zzz = 0x0002; // useful for sleeping or lip-sealed textbox moments
    static const vfx fast = 0x0003; // doubles the text play-out animation for intense moments
  };

private:
  const double DRAMATIC_TEXT_SPEED = 0.05;
  const double FAST_TEXT_SPEED = 2.0;

  vfx currEffect{};

  Font font;
  mutable Text text;
  double charsPerSecond; /**< default is 40 cps */
  double progress; /**< Total elapsed time */
  int areaWidth, areaHeight;
  std::string message;
  std::vector<int> lines; /**< Precalculated. List of all line start places. */
  std::vector<int> insertedNewLines; /**< Precalculated. List of all inserted new lines. */
  int lineIndex; /**< Index of the current line being typed. */
  int numberOfFittingLines; /**< Precalculated. Number of fitting lines in the area. */
  int charIndex; /**< The current character index in the entire message. */
  bool play{}; /**< If true, types out message. If false, pauses. */
  bool mute{}; /**< Enables a sound to play every time a character is printed */
  bool dirty{};
  int charSize; /**< Font size */

  /**
   * @brief Takes the input message and finds where the text breaks to form new lines
   */
  void FormatToFit();
  void StoreCurrentBlock();

  const bool ProcessSpecialCharacters(int& pos);

  std::pair<size_t, size_t> GetCurrentCharacterRangeRaw() const;
  std::pair<size_t, size_t> GetBlockCharacterRangeRaw() const;

public:
  /**
   * @brief Creates a textbox area of width x height, default font size 15, and hard-coded font path
   * @param width in pixels
   * @param height in pixels
   */
   TextBox(int width, int height);
   TextBox(int width, int height, const Font& font);

  ~TextBox();

  /**
   * @brief Get reference to Text object
   * @return Text&
   */
  const Text& GetText() const;

  const vfx GetVFX() const;

  /**
 * @brief Get reference to Font object
 * @return Font&
 */
  const Font& GetFont() const;

  /**
   * @brief Set text fill color
   * @param color
   */
  void SetTextFillColor(sf::Color color);

  /**
   * @brief Set both text outline and fill color
   * @param color
   */
  void SetTextColor(sf::Color color);

  /**
   * @brief Disables Audio()
   * @param enabled Default is true
   */
  void Mute(bool enabled = true);

  /**
   * @brief Enables Audio()
   */
  void Unmute();

  /**
   * @brief Query if there's more text to show
   * @return true if there's more text to show, false if all visible text is printed
   */
  const bool HasMore() const;

  /**
   * @brief Query if there's text behind what is currently shown
   * @return true if lineIndex > 0
   */
  const bool HasLess() const;

  /**
   * @brief Moves lines upward in text area, shows next lines
   */
  void ShowNextLine();

  /**
   * @brief Moves lines downward in text area, shows previous lines
   */
  void ShowPreviousLine();

  /**
  * @brief Writes-out the currently unrolling line and effectively skips the animation
  */
  void CompleteCurrentBlock();

  /**
  * @brief skips all animation. Useful if you just want to show text.
  */
  void CompleteAll();

  /**
   * @brief Change how many characters are printed per second
   * @param cps
   */
  void SetCharactersPerSecond(const double cps);

  /**
   * @brief Set the textbox message. Text is automatically formatted.
   * @param message
   */
  void SetText(const std::string& text);

  /**
   * @brief Plays text printing
   * @param play
   */
  void Play(const bool play = true);

  /**
   * @brief Pauses text printing
   */
  void Stop();

  /**
   * @brief Returns the current character printed
   * @return char
   */
  const uint32_t GetCurrentCharacter() const;

  /**
   * @brief Returns the character range of the displayed text
   * @return pair of size_t
   */
  std::pair<size_t, size_t> GetCurrentCharacterRange() const;

  /**
   * @brief Returns the range of the displayed lines (not the range of the characters)
   * @return pair of size_t
   */
  std::pair<size_t, size_t> GetCurrentLineRange() const;

  /**
   * @brief Returns the character range of the completed displayed text
   * @return pair of size_t
   */
  std::pair<size_t, size_t> GetBlockCharacterRange() const;

  /**
   * @brief Returns the number of fitting lines
   * @return int
   */
  const int GetNumberOfFittingLines() const;

  /**
   * @brief Return count of all formatted line
   * @return int
   */
  const int GetNumberOfLines() const;

  /**
   * @brief Get current characters printed per sec
   * @return double in sec
   */
  const double GetCharsPerSecond() const;

  /**
   * @brief Query if textbox is playing
   * @return true if playing, false if paused
   */
  const bool IsPlaying() const;

  virtual void Update(const double elapsed);

  /**
   * @brief Query if the textbox has reached the end of the message
   * @return charIndex >= message.length()
   */
  const bool IsEndOfMessage() const;

  const bool IsEndOfBlock() const;

  bool IsFinalBlock() const;

  /**
   * @brief Draws the textbox with correct transformations
   *
   * The textbox inherits sf::Drawable which give it transform properties
   * and can be rotated and scaled. We make sure the text is rendered appropriately.
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};
