#include "bnTextBox.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

namespace {
  const char dramatic_token = '\x01';
  const char nolip_token = '\x02';
  const auto special_chars = { ::nolip_token, ::dramatic_token, ' ',  '\n' };
}

TextBox::TextBox(int width, int height) :
  TextBox::TextBox(width, height, Font::Style::thin) { } 

TextBox::TextBox(int width, int height, const Font& font) : 
  font(font), 
  text("", font) {
  text.scale(2.0f, 2.0f);
  message = "";
  areaWidth = width;
  areaHeight = height;
  charsPerSecond = 10;
  charIndex = 0;
  play = true;
  mute = false;
  progress = 0;
  lineIndex = 0;
  numberOfFittingLines = 1;
}

TextBox::~TextBox() {
}

void TextBox::FormatToFit() {
  if (message.empty())
    return;

  message = replace(message, "\\n", "\n"); // replace all ascii "\n" to carriage return char '\n'
  message = replace(message, "\\x01", "\x01"); // replace ascii
  message = replace(message, "\\x02", "\x02"); // replace ascii

  lines.push_back(0); // All text begins at pos 0

  text.SetString(message);

  int index = 0;
  int wordIndex = -1; // If we are breaking on a word
  int lastRow = 0;
  int line = 1;

  double fitHeight = 0;

  while (index < message.size()) {
    auto iter = std::find(::special_chars.begin(), ::special_chars.end(), message[index]);
    if (iter == special_chars.end()) {
      if (wordIndex == -1) { // only mark the beginning of a word
        wordIndex = index;
      }
    }
    else {
      wordIndex = -1;
    }

    std::string fitString = message.substr(lastRow, (size_t)index - (size_t)lastRow);
    fitString = replace(fitString, "\n", ""); // line breaks shouldn't increase real estate...
    fitString = replace(fitString, std::string(1, ::nolip_token), ""); // fx sections shouldn't increase real estate...
    fitString = replace(fitString, std::string(1, ::dramatic_token), ""); //fx sections shouldn't increase real estate...
    text.SetString(fitString);

    double width = text.GetWorldBounds().width;
    double height = text.GetWorldBounds().height;

    if (message[index] == '\n') {

      if (wordIndex > 0) {
        lastRow = wordIndex + 1;
      }
      else {
        lastRow = index;
      }

      lines.push_back(index + 1);

      if (fitHeight <= areaHeight) {
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

      if (fitHeight <= areaHeight) {
        line++;
        fitHeight += height;
      }
    }
    index++;
  }

  // make final text blank to start
  text.SetString("");

  numberOfFittingLines = line;
}

std::string TextBox::replace(std::string str, const std::string& from, const std::string& to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}


const Text& TextBox::GetText() const { return text; }

const TextBox::vfx TextBox::GetVFX() const
{
  return currEffect;
}

const Font& TextBox::GetFont() const { return font; }

void TextBox::SetTextFillColor(sf::Color color) {
  text.SetColor(color);
}

void TextBox::SetTextOutlineColor(sf::Color color) {
  // outlineColor = color;
}

void TextBox::SetTextColor(sf::Color color) {
  SetTextFillColor(color);
  SetTextOutlineColor(color);
}

void TextBox::Mute(bool enabled) {
  mute = enabled;
}

void TextBox::Unmute() {
  Mute(false);
}

const bool TextBox::HasMore() const {
  if ((size_t)lineIndex + (size_t)numberOfFittingLines < lines.size()) {
    if (charIndex > lines[(size_t)lineIndex + (size_t)numberOfFittingLines]) {
      return true;
    }
  }

  return false;
}

const bool TextBox::HasLess() const {
  return lineIndex > 0;
}

void TextBox::ShowNextLine() {
  lineIndex++;

  if (lineIndex >= lines.size())
    lineIndex = (int)lines.size() - 1;

  dirty = true;
}

void TextBox::ShowPreviousLine() {
  lineIndex--;

  if (lineIndex < 0)
    lineIndex = 0;

  dirty = true;
}

void TextBox::CompleteCurrentBlock()
{
  size_t newCharIndex = message.size() - 1;
  int lastLine = lineIndex + GetNumberOfFittingLines();

  if (lastLine < this->lines.size()) {
    newCharIndex = this->lines[lastLine];
  }

  int charactersSkipped = 0;
  int index = charIndex;

  // do not count spaces
  while (index < newCharIndex) {
    if (message[index] == ' ') {
      index++;
      continue;
    }

    index++;
    charactersSkipped++;
  }

  double elapsed = static_cast<double>(charactersSkipped) / this->charsPerSecond;
  this->progress += elapsed;

  dirty = true; // will try and pause once it completes, so we force it to update
}

void TextBox::CompleteAll()
{
  charIndex = static_cast<int>(message.length());

  dirty = true; // will try and pause once it completes, so we force it to update
}

void TextBox::SetCharactersPerSecond(const double cps) {
  charsPerSecond = cps;
}

void TextBox::SetText(const std::string& text) {
  message = text;
  charIndex = 0;
  progress = 0;
  lines.clear();
  lineIndex = 0;
  numberOfFittingLines = 1;
  currEffect = TextBox::effects::none;
  FormatToFit();
  dirty = true;
}

void TextBox::Play(const bool play) {
  TextBox::play = play;
}

void TextBox::Stop() {
  Play(false);
}

const char TextBox::GetCurrentCharacter() const {
  if (text.GetString().empty()) return '\0';

  return text.GetString().back();
}

const int TextBox::GetNumberOfFittingLines() const {
  return numberOfFittingLines;
}

const int TextBox::GetNumberOfLines() const {
  return (int)lines.size();
}

const double TextBox::GetCharsPerSecond() const {
  return charsPerSecond;
}

const bool TextBox::IsPlaying() const {
  return play;
}

void TextBox::Update(const double elapsed) {
  // If the message is empty don't update
  if (message.empty()) return;

  // If we're paused don't update
  // If the text is set don't update the first frame
  if (!play && !dirty) return;

  // If we're at the end of the message, don't step  
  // through the words
  if (charIndex >= message.length()) {
    int begin = lines[lineIndex];
    int lastIndex = std::min((int)lines.size() - 1, lineIndex + numberOfFittingLines - 1);
    int last = lines[lastIndex];
    int len = 0;

    size_t pos = static_cast<size_t>(lineIndex) + static_cast<size_t>(numberOfFittingLines);
    if (pos < lines.size()) {
      len = std::min(charIndex - begin, lines[pos] - begin);
    }
    else {
      len = charIndex - begin;
    }

    text.SetString(message.substr(begin, len));

    play = false;

    // We don't need to fill box
    return;
  }

  // Without this, the Audio() would play numerous times per frame and sounds bad
  bool playOnce = true;

  int charIndexIter = charIndex;
  progress += elapsed;

  double modifiedCharsPerSecond = charsPerSecond;

  if ((currEffect & effects::dramatic) == effects::dramatic) {
    // if we are doing dramatic text, reduce speed
    modifiedCharsPerSecond = 0.5;
  }

  while (modifiedCharsPerSecond > 0 && progress > 1.0/modifiedCharsPerSecond) {
    progress -= 1.0 / modifiedCharsPerSecond;

    auto checkSpecialCharacters = [this](int& pos) {
      if (pos >= message.size()) return false;

      bool processed = false;

      // Skip over line breaks and empty spaces
      auto iter = std::find(::special_chars.begin(), ::special_chars.end(), message[pos]);
      while (iter != ::special_chars.end()) {
        processed = true;

        // The following conditions handles elipses for dramatic effect
        if (message[pos] == ::dramatic_token) {
          currEffect ^= effects::dramatic;
        }
        else if (message[pos] == ::nolip_token) {
          // this makes mugshots stop animating (for thinking effects)
          currEffect ^= effects::zzz;
        }

        pos++;

        if (pos >= message.size()) break;

        iter = std::find(::special_chars.begin(), ::special_chars.end(), message[pos]);
      }

      return processed;
    };

    // Try the next character
    if (!checkSpecialCharacters(charIndexIter)) {
      charIndexIter++;
      checkSpecialCharacters(charIndexIter);
    }

    // If we're passed the current char index but there's more text to show...
    if (charIndexIter > charIndex && charIndex < message.size()) {
      // See how many non-spaces there were in this pass
      int length = charIndexIter - charIndex;
      std::string pass = message.substr(charIndex, length+1);
      bool talking = pass.end() != std::find_if(pass.begin(), pass.end(), [](char in) { 
        auto iter = std::find(::special_chars.begin(), ::special_chars.end(), in);
        return iter == ::special_chars.end();
      });

      if (talking) {
        // Play a sound if we are able and the character is a letter
        if (!mute && playOnce) {
          Audio().Play(AudioType::TEXT, AudioPriority::high);
          playOnce = false;
        }
      }

      // Update the current char index
      charIndex = charIndexIter;
    }

    int begin = lines[lineIndex];
    int len = begin;

    /** This section limits the visible string
    * if the text has already been printed or if
    * the text is currently being printed. The visible line
    * may change if the user request ShowNextLine() or ShowPreviousLine()
    * We make sure we show the last visible character in the line*/

    if (charIndex >= lines[lineIndex]) {
      size_t pos = static_cast<size_t>(lineIndex) + static_cast<size_t>(numberOfFittingLines);
      if (pos < lines.size()) {
        len = std::min(charIndex - begin, lines[pos] - begin);
      }
      else {
        len = charIndex - begin;
      }
    }
    else {
      len = 0;
    }

    // Len will be > 0 after first call to Update()
    // Set the sf::Text to show only the visible text in 
    // the text area
    if (len <= 0) {
      text.SetString("");
    }
    else {
      size_t count = (size_t)len + 1;
      std::string outString = message.substr(begin, count);
      outString = replace(outString, std::string(1, ::nolip_token), ""); // fx should not appear in final message
      outString = replace(outString, std::string(1, ::dramatic_token), ""); // fx should not appear in final message
      text.SetString(outString);
    }
  }

  dirty = false;
}

const bool TextBox::IsEndOfMessage() const {
    return (charIndex >= message.length());
}

const bool TextBox::IsEndOfBlock() const
{
  int testCharIndex = static_cast<int>(message.size()) - 1;
  int lastLine = lineIndex + GetNumberOfFittingLines();

  if (lastLine < this->lines.size()) {
    testCharIndex = this->lines[lastLine] - 1;
  }
  
  return charIndex >= testCharIndex;
}

void TextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (message.empty())
    return;

  // apply the transform
  states.transform *= getTransform();

  target.draw(text, states);
}