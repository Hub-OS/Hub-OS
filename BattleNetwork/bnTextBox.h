/*! \brief Define a rectangular area, font size, print speed, and a message that types out over time
 */

#pragma once

#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <SFML/Graphics.hpp>

class TextBox : public sf::Drawable, public sf::Transformable {
private:
  sf::Font* font;
  mutable sf::Text text;
<<<<<<< HEAD
  double charsPerSecond; // default is 10 cps
  double progress;
  int areaWidth, areaHeight;
  std::string message;
  std::vector<int> lines;
  int lineIndex;
  int numberOfFittingLines;
  int charIndex;
  bool play;
  bool mute;
  int charSize;
  sf::Color fillColor;
  sf::Color outlineColor;


=======
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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void FormatToFit() {
    if (message.empty())
      return;

    message = replace(message, "\\n", "\n"); // replace all ascii "\n" to carriage return char '\n'

    lines.push_back(0); // All text begins at pos 0

    text = sf::Text(message, *font);
    text.setCharacterSize(charSize);

    sf::Text prevText = text;

    int index = 0;
    int wordIndex = -1; // If we are breaking on a word
    int lastRow = 0;
    int line = 1;

    double fitHeight = 0;

    while (index < message.size()) {
      if (message[index] != ' ' && message[index] != '\n' && wordIndex == -1) {
        wordIndex = index;
      }
      else if (message[index] == ' ') {
        wordIndex = -1;
      }

      text.setString(message.substr(lastRow, index - lastRow));

      double width = text.getGlobalBounds().width;
      double height = text.getGlobalBounds().height;

      if (message[index] == '\n' && wordIndex != -1) {
        lastRow = wordIndex + 1;
        lines.push_back(index + 1);

        if (fitHeight < areaHeight) {
          line++;
          fitHeight += height;
        }

        wordIndex = -1;

      }
      else if (width > areaWidth && wordIndex != -1 && wordIndex > 0 && index > 0) {
        // Line break at the next word
        message.insert(wordIndex, "\n");
        lastRow = wordIndex + 1;
        lines.push_back(lastRow);
        index = lastRow;
        wordIndex = -1;

        if (fitHeight < areaHeight) {
          line++;
          fitHeight += height;
        }
      }
      index++;
    }

    // make final text blank to start
    text.setString("");

    numberOfFittingLines = line;
  }

  /**
   * @brief Replaces matching string `from` to `to` in source string `str`
   * @param str source string
   * @param from matching pattern
   * @param to replace with
   * @return modified string
   */
  std::string replace(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
  }

<<<<<<< HEAD
    std::cout << "num of fitting lines: " << numberOfFittingLines << std::endl;
    std::cout << "lines found: " << lines.size() << std::endl;
  }

  std::string replace(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
  }

public:
  TextBox(int width, int height, int characterSize = 15, std::string fontPath = "resources/fonts/dr_cain_terminal.ttf") {
    font = TEXTURES.LoadFontFromFile(fontPath);
    text = sf::Text();
    message = "";
    areaWidth = width;
    areaHeight = height;
    charsPerSecond = 10;
    charIndex = 0;
    play = true;
    mute = false;
    progress = 0;
    charSize = characterSize;
    fillColor = sf::Color::White;
    outlineColor = sf::Color::White;
    lineIndex = 0;
    numberOfFittingLines = 1;
  }

  ~TextBox() {
    delete font;
  }

  const sf::Text& GetText() const { return this->text; }

  void SetTextFillColor(sf::Color color) {
    fillColor = color;
  }

  void SetTextOutlineColor(sf::Color color) {
    outlineColor = color;
  }

  void SetTextColor(sf::Color color) {
    SetTextFillColor(color);
    SetTextOutlineColor(color);
  }

  void Mute(bool enabled = true) {
    mute = enabled;
  }

=======
public:
  /**
   * @brief Creates a textbox area of width x height, default font size 15, and hard-coded font path
   * @param width in pixels
   * @param height in pixels
   * @param characterSize font size
   * @param fontPath default "resources/fonts/dr_cain_terminal.ttf"
   */
  TextBox(int width, int height, int characterSize = 15, std::string fontPath = "resources/fonts/dr_cain_terminal.ttf") {
    font = TEXTURES.LoadFontFromFile(fontPath);
    text = sf::Text();
    message = "";
    areaWidth = width;
    areaHeight = height;
    charsPerSecond = 10;
    charIndex = 0;
    play = true;
    mute = false;
    progress = 0;
    charSize = characterSize;
    fillColor = sf::Color::White;
    outlineColor = sf::Color::White;
    lineIndex = 0;
    numberOfFittingLines = 1;
  }

  ~TextBox() {
    delete font;
  }

  /**
   * @brief Get sf::Text object
   * @return sf::Text&
   */
  const sf::Text& GetText() const { return this->text; }

  /**
   * @brief Set text fill color
   * @param color
   */
  void SetTextFillColor(sf::Color color) {
    fillColor = color;
  }

  /**
   * @brief Set text outline color
   * @param color
   */
  void SetTextOutlineColor(sf::Color color) {
    outlineColor = color;
  }

  /**
   * @brief Set both text outline and fill color
   * @param color
   */
  void SetTextColor(sf::Color color) {
    SetTextFillColor(color);
    SetTextOutlineColor(color);
  }

  /**
   * @brief Disables audio
   * @param enabled Default is true
   */
  void Mute(bool enabled = true) {
    mute = enabled;
  }

  /**
   * @brief Enables audio
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Unmute() {
    Mute(false);
  }

<<<<<<< HEAD

=======
  /**
   * @brief Query if there's more text to show
   * @return true if there's more text to show, false if all visible text is printed
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  const bool HasMore() const {
    if (lineIndex + numberOfFittingLines < lines.size())
      if (charIndex > lines[lineIndex + numberOfFittingLines])
        return true;

    return false;
  }

<<<<<<< HEAD
=======
  /**
   * @brief Query if there's text behind what is currently shown
   * @return true if lineIndex > 0
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  const bool HasLess() const {
    return lineIndex > 0;
  }

  /**
   * @brief Moves lines upward in text area, shows next lines
   */
  void ShowNextLine() {
    lineIndex++;

    if (lineIndex >= lines.size())
      lineIndex = (int)lines.size()-1;
<<<<<<< HEAD

  }
=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  }
  
  /**
   * @brief Moves lines downward in text area, shows previous lines
   */
  void ShowPreviousLine() {
    lineIndex--;

    if (lineIndex < 0)
      lineIndex = 0;
  }

<<<<<<< HEAD
=======
  /**
   * @brief Change how many characters are printed per second
   * @param cps
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void SetCharactersPerSecond(const double cps) {
    this->charsPerSecond = cps;
  }

  /**
   * @brief Set the textbox message. Text is automatically formatted.
   * @param message
   */
  void SetMessage(const std::string message) {
    this->message = message;
    charIndex = 0;
    progress = 0;
    lines.clear();
    lineIndex = 0;
    numberOfFittingLines = 1;
    FormatToFit();
  }

  /**
   * @brief Plays text printing
   * @param play
   */
  void Play(const bool play = true) {
    this->play = play;
  }

  /**
   * @brief Pauses text printing
   */
  void Stop() {
    Play(false);
  }

<<<<<<< HEAD
  const char GetCurrentCharacter() const {
    return message[charIndex];
  }

  const int GetNumberOfFittingLines() const {
    return numberOfFittingLines;
  }

  const int GetNumberOfLines() const {
    return (int)lines.size();
  }

  const double GetCharsPerSecond() const {
    return charsPerSecond;
  }
  
  const bool IsPlaying() const {
	  return play;
  }

  virtual void Update(const double elapsed) {
    if (!play || message.empty() || charIndex >= message.length()) return;

=======
  /**
   * @brief Returns the current character printed
   * @return char
   */
  const char GetCurrentCharacter() const {
    return message[charIndex];
  }

  /**
   * @brief Returns the number of fitting lines
   * @return int
   */
  const int GetNumberOfFittingLines() const {
    return numberOfFittingLines;
  }

  /**
   * @brief Return count of all formatted line
   * @return int
   */
  const int GetNumberOfLines() const {
    return (int)lines.size();
  }

  /**
   * @brief Get current characters printed per sec
   * @return double in sec
   */
  const double GetCharsPerSecond() const {
    return charsPerSecond;
  }
  
  /**
   * @brief Query if textbox is playing
   * @return true if playing, false if paused
   */
  const bool IsPlaying() const {
	  return play;
  }

  virtual void Update(const double elapsed) {
    // If we're paused don't update
    // If the message is empty don't update
    // If we're at the end of the message, don't update
    if (!play || message.empty() || charIndex >= message.length()) return;

    // Without this, the audio would play numerous times per frame and sounds bad
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    bool playOnce = true;

    int charIndexIter = 0;
    progress += elapsed;

    // Work backwards, printing what we can show at this frame in respect to the CPS
    double simulate = progress;
<<<<<<< HEAD
    while (simulate > 0 && charsPerSecond > 0) {
      simulate -= 1.0/ charsPerSecond;

=======
    
    // Start at elapsed time `progress` and simulate until it his zero
    // That is our new state
    while (simulate > 0 && charsPerSecond > 0) {
        
      simulate -= 1.0/ charsPerSecond;
 
      // Skip over line breaks and empty spaces
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      while (charIndexIter < message.size() && message[charIndexIter] == ' ' && message[charIndex] != '\n') {
        charIndexIter++;
      }

<<<<<<< HEAD
      charIndexIter++;

      if (charIndexIter > charIndex && charIndex < message.size()) {
        charIndex = charIndexIter;

=======
      // Try the next character
      charIndexIter++;

      // If we're passed the current char index but there's more text to show...
      if (charIndexIter > charIndex && charIndex < message.size()) {
          
        // Update the current char index
        charIndex = charIndexIter;

        // We may overshoot, adjust
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
        if (charIndexIter >= message.size()) {
          charIndex--;
        }
        else {
<<<<<<< HEAD

          std::cout << message[charIndex];

=======
          // Play a sound if we are able and the character is a letter
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
          if (!mute && message[charIndex] != ' ' && message[charIndex] != '\n') {
            if (playOnce) {
              AUDIO.Play(AudioType::TEXT);
              playOnce = false;
            }
          }
        }
      }
      
      int begin = lines[lineIndex];
      int len = begin;

      /** This section limits the visible string 
       * if the text has already been printed or if 
       * the text is currently being printed. The visible line
       * may change if the user request ShowNextLine() or ShowPreviousLine()
       * We make sure we show the last visible character in the line*/
       
      if (charIndex >= lines[lineIndex]) {
        if (lineIndex + (numberOfFittingLines) < lines.size()) {
          len = std::min(charIndex - begin, lines[lineIndex + (numberOfFittingLines)] - begin);
        }
        else {
          len = charIndex - begin;
        }
      }
      else
        len = 0;

      // Len will be > 0 after first call to Update()
      // Set the sf::Text to show only the visible text in 
      // the text area
      if (len <= 0)
        text.setString("");
      else 
        text.setString(message.substr(begin, len));
    }
  }

<<<<<<< HEAD
=======
  /**
   * @brief Query if the textbox has reached the end of the message
   * @return charIndex >= message.length()
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  const bool EndOfMessage() const { 
    return (charIndex >= message.length()); 
  }

<<<<<<< HEAD
=======
  /**
   * @brief Draws the textbox with correct transformations
   * 
   * The textbox inherits sf::Drawable which give it transform properties
   * and can be rotated and scaled. We make sure the text is rendered appropriately.
   * @param target
   * @param states
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
  {
    if (message.empty())
      return;

    // apply the transform
    states.transform *= getTransform();

    text.setPosition(this->getPosition());
    text.setScale(this->getScale());
    text.setRotation(this->getRotation());
    text.setFillColor(fillColor);
    text.setOutlineColor(outlineColor);

    target.draw(text);
  }
};
