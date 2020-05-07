/*! \brief Define a rectangular area, font size, print speed, and a message that types out over time
 */

#pragma once

#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <SFML/Graphics.hpp>
#include <memory>

class TextBox : public sf::Drawable, public sf::Transformable {
private:
  std::shared_ptr<sf::Font> font;
  mutable sf::Text text;
  double charsPerSecond; /**< default is 10 cps */
  double progress; /**< Total elapsed time */
  int areaWidth, areaHeight;
  std::string message;
  std::vector<int> lines; /**< Precalculated. List of all line start places. */
  int lineIndex; /**< Index of the current line being typed. */
  int numberOfFittingLines; /**< Precalculated. Number of fitting lines in the area. */
  int charIndex; /**< The current character index in the entire message. */
  bool play; /**< If true, types out message. If false, pauses. */
  bool mute; /**< Enables a sound to play every time a character is printed */
  int charSize; /**< Font size */
  sf::Color fillColor; /**< Fill color of text */
  sf::Color outlineColor; /**< Outline color of text */

  /**
   * @brief Takes the input message and finds where the text breaks to form new lines
   */
  void FormatToFit();

  /**
   * @brief Replaces matching string `from` to `to` in source string `str`
   * @param str source string
   * @param from matching pattern
   * @param to replace with
   * @return modified string
   */
  std::string replace(std::string str, const std::string& from, const std::string& to);

public:
  /**
   * @brief Creates a textbox area of width x height, default font size 15, and hard-coded font path
   * @param width in pixels
   * @param height in pixels
   * @param characterSize font size
   * @param fontPath default "resources/fonts/dr_cain_terminal.ttf"
   */
   TextBox(int width, int height, int characterSize = 15, std::string fontPath = "resources/fonts/dr_cain_terminal.ttf");

  ~TextBox();

  /**
   * @brief Get reference to sf::Text object
   * @return sf::Text&
   */
  const sf::Text& GetText() const;

  /**
 * @brief Get reference to sf::Font object
 * @return sf::Font&
 */
  const sf::Font& GetFont() const;

  /**
   * @brief Set text fill color
   * @param color
   */
  void SetTextFillColor(sf::Color color);

  /**
   * @brief Set text outline color
   * @param color
   */
  void SetTextOutlineColor(sf::Color color);

  /**
   * @brief Set both text outline and fill color
   * @param color
   */
  void SetTextColor(sf::Color color);

  /**
   * @brief Disables audio
   * @param enabled Default is true
   */
  void Mute(bool enabled = true);

  /**
   * @brief Enables audio
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
  const char GetCurrentCharacter() const;

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
  const bool EndOfMessage() const;

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
