#include "bnTextBox.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "stx/string.h"

#include <string_view>
#include <Poco/UTF8Encoding.h>

namespace {
  const char dramatic_token = '\x01';
  const char nolip_token = '\x02';
  const char fast_token = '\x03';
  const auto special_chars = { ::nolip_token, ::dramatic_token, ' ',  '\n' };

  stx::result_t<std::pair<uint32_t, size_t>> nextUTF8CodepointAndSize(std::string_view s) {
    Poco::UTF8Encoding utf8Encoding;
    size_t size = 1;
    for (;;) {
      if (size > s.size()) {
        return stx::error<std::pair<uint32_t, size_t>>("premature end of bytes");
      }

      int r = utf8Encoding.queryConvert(reinterpret_cast<const unsigned char*>(s.data()), size);
      if (r == -1) {
        return stx::error<std::pair<uint32_t, size_t>>("malformed byte sequence");
      }

      if (r > 0) {
        return std::make_pair(static_cast<uint32_t>(r), size);
      }

      size = -r;
    }
  }

  size_t nextValidUTF8CodepointIndex(std::string_view s) {
    for (size_t i = 0; i < s.size(); ++i) {
      stx::result_t<std::pair<uint32_t, size_t>> r = nextUTF8CodepointAndSize(s.substr(i));
      if (!r.is_error()) {
        return i + r.value().second;
      }
    }
    return s.size();
  }

  uint32_t lastUTF8Codepoint(std::string_view s) {
    // The cursed UTF-8 backwards iteration routine. Based on https://stackoverflow.com/a/22257843.
    if (s.empty()) {
      return 0;
    }

    size_t i = s.size() - 1;
    while (i >= 0 && (s[i] & 0xc0) == 0x80) {
      --i;
    }

    stx::result_t<std::pair<uint32_t, size_t>> r = nextUTF8CodepointAndSize(s.substr(i));
    if (r.is_error()) {
      return 0;
    }

    return r.value().first;
  }
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
  charsPerSecond = 40;
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

  message = stx::replace(message, "\\n", "\n"); // replace all ascii "\n" to carriage return char '\n'
  message = stx::replace(message, "\\x01", "\x01"); // replace ascii
  message = stx::replace(message, "\\x02", "\x02"); // replace ascii

  insertedNewLines.clear();
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

    std::string fitString = message.substr(lastRow, (size_t)index - (size_t)lastRow + nextValidUTF8CodepointIndex(std::string_view(message).substr(index)));

    // fx sections shouldn't increase real estate...
    fitString = stx::replace(fitString, std::string(1, ::nolip_token), "");
    fitString = stx::replace(fitString, std::string(1, ::dramatic_token), "");
    fitString = stx::replace(fitString, std::string(1, ::fast_token), "");

    text.SetString(fitString);

    double width = text.GetWorldBounds().width;

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
        fitHeight += font.GetLineHeight() * text.getScale().y;
      }

      wordIndex = -1;

    }
    else if (width > areaWidth && wordIndex != -1 && wordIndex > lastRow + 1) {
      // Line break at the next word
      message.insert(wordIndex, "\n");

      lastRow = wordIndex + 1;
      lines.push_back(lastRow);
      insertedNewLines.push_back(lastRow);
      index = lastRow;
      wordIndex = -1;

      if (fitHeight <= areaHeight) {
        line++;
        fitHeight += font.GetLineHeight() * text.getScale().y;
      }
    }
    else if (width > areaWidth) {
      // if we're on a space place the new line after
      if (message[index] == ' ') {
        index += nextValidUTF8CodepointIndex(std::string_view(message).substr(index));
      }

      lastRow = index;
      message.insert(lastRow, "\n");
      lines.push_back(lastRow + 1);
      insertedNewLines.push_back(lastRow + 1);

      if (fitHeight <= areaHeight) {
        line++;
        fitHeight += font.GetLineHeight() * text.getScale().y;
      }

      wordIndex = -1;
    }
    index += nextValidUTF8CodepointIndex(std::string_view(message).substr(index));
  }

  // make final text blank to start
  text.SetString("");

  numberOfFittingLines = line;
}

const bool TextBox::ProcessSpecialCharacters(int& pos) {
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
    else if (message[pos] == ::fast_token) {
      currEffect ^= effects::fast;
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

const Text& TextBox::GetText() const { return text; }

const TextBox::vfx TextBox::GetVFX() const
{
  return currEffect;
}

const Font& TextBox::GetFont() const { return font; }

void TextBox::SetTextFillColor(sf::Color color) {
  text.SetColor(color);
}

void TextBox::SetTextColor(sf::Color color) {
  SetTextFillColor(color);
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
  if (lineIndex >= lines.size()) return;

  // simulate the end of the text block starting from the beginning
  // and compile a list of flags and set as complete
  int start = lines[lineIndex];
  int end = (int)message.size();
  size_t block_end = (size_t)lineIndex + numberOfFittingLines;
  if (block_end < lines.size()) {
    end = lines[block_end];
  }

  currEffect = effects::none;

  double simProgress = 0;

  while (start < end) {
    double modifiedCharsPerSecond = charsPerSecond;

    if (!ProcessSpecialCharacters(start)) {
      start++;
    }

    if ((currEffect & effects::dramatic) == effects::dramatic) {
      modifiedCharsPerSecond = charsPerSecond * DRAMATIC_TEXT_SPEED;
    }
    else if ((currEffect & effects::fast) == effects::fast) {
      modifiedCharsPerSecond = charsPerSecond * FAST_TEXT_SPEED;
    }

    simProgress += 1.0 / modifiedCharsPerSecond;
  }

  charIndex = end;
  progress = simProgress;

  StoreCurrentBlock();
}

void TextBox::CompleteAll()
{
  charIndex = static_cast<int>(message.length());

  dirty = true;
  // below code can be used to resolve flickering, use instead of the dirty = true
  // untested

  // lineIndex = lines.size() - 3;

  // StoreCurrentBlock();
}

void TextBox::StoreCurrentBlock() {
  auto [begin, end] = GetCurrentCharacterRangeRaw();

  text.SetString(message.substr(begin, end - begin));

  play = false;
  dirty = false;
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

const uint32_t TextBox::GetCurrentCharacter() const {
  return lastUTF8Codepoint(text.GetString());
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

std::pair<size_t, size_t> TextBox::GetCurrentCharacterRangeRaw() const {
  int begin = lines[lineIndex];
  int end = 0;

  size_t pos = static_cast<size_t>(lineIndex + numberOfFittingLines);

  if (pos < lines.size()) {
    end = std::min(charIndex, lines[pos]);
  }
  else {
    end = charIndex;
  }

  return { begin, end };
}

// adjusts for inserted newlines
std::pair<size_t, size_t> TextBox::GetCurrentCharacterRange() const {
  auto range = GetCurrentCharacterRangeRaw();

  auto [begin, end] = range;

  for (auto index : insertedNewLines) {
    if (begin >= index) range.first -= 1;
    if (end >= index) range.second -= 1;
  }

  return range;
}

std::pair<size_t, size_t> TextBox::GetCurrentLineRange() const {
  if (lines.size() == 0) {
    return { 0, 0 };
  }

  return { lineIndex, std::min((size_t)(lineIndex + numberOfFittingLines - 1), lines.size() - 1) };
}

std::pair<size_t, size_t> TextBox::GetBlockCharacterRangeRaw() const {
  size_t begin = (size_t)lines[lineIndex];
  size_t end = 0;

  size_t pos = static_cast<size_t>(lineIndex + numberOfFittingLines);
  end = pos < lines.size() ? static_cast<size_t>(lines[pos]) : message.length();

  return { begin, end };
}

// adjusts for inserted newlines
std::pair<size_t, size_t> TextBox::GetBlockCharacterRange() const {
  auto range = GetBlockCharacterRangeRaw();

  auto [begin, end] = range;

  for (auto index : insertedNewLines) {
    if (begin >= index) range.first -= 1;
    if (end >= index) range.second -= 1;
  }

  return range;
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
    StoreCurrentBlock();

    // We don't need to fill box
    return;
  }

  // Without this, the audio fx would play numerous times per frame and sounds bad
  bool playOnce = true;

  int charIndexIter = charIndex;
  progress += elapsed;

  double modifiedCharsPerSecond = charsPerSecond;

  if ((currEffect & effects::dramatic) == effects::dramatic) {
    // if we are doing dramatic text, reduce speed
    modifiedCharsPerSecond = charsPerSecond * DRAMATIC_TEXT_SPEED;
  }
  else if ((currEffect & effects::fast) == effects::fast) {
    modifiedCharsPerSecond = charsPerSecond * FAST_TEXT_SPEED;
  }

  while (modifiedCharsPerSecond > 0 && progress > 1.0/modifiedCharsPerSecond) {
    progress -= 1.0 / modifiedCharsPerSecond;

    // Try the next character
    if (!ProcessSpecialCharacters(charIndexIter)) {
      charIndexIter+=nextValidUTF8CodepointIndex(std::string_view(message).substr(charIndexIter));
      ProcessSpecialCharacters(charIndexIter);
    }

    // If we're passed the current char index but there's more text to show...
    if (charIndexIter > charIndex && charIndex < message.size()) {
      // See how many non-spaces there were in this pass
      int length = charIndexIter - charIndex;
      std::string pass = message.substr(charIndex, (size_t)length+nextValidUTF8CodepointIndex(std::string_view(message).substr(charIndex+length)));
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
        len = std::min(charIndex - begin, lines[pos] - begin) - 1;
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
      std::string outString = message.substr(begin, (size_t)len+nextValidUTF8CodepointIndex(std::string_view(message).substr(begin+len)));
      outString = stx::replace(outString, std::string(1, ::nolip_token), ""); // fx should not appear in final message
      outString = stx::replace(outString, std::string(1, ::dramatic_token), ""); // fx should not appear in final message
      text.SetString(outString);
      progress = 0; // don't roll over
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

bool TextBox::IsFinalBlock() const {
  return lines.size() == 0 || GetCurrentLineRange().second == lines.size() - 1;
}

void TextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (message.empty())
    return;

  // apply the transform
  states.transform *= getTransform();

  target.draw(text, states);
}
