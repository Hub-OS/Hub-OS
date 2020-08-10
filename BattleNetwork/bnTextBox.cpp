#include "bnTextBox.h"

void TextBox::FormatToFit() {
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
        if (message[index] != ' ' && message[index] != '\n') {
            if (wordIndex == -1) { // only mark the beginning of a word
                wordIndex = index;
            }
        }
        else {
            wordIndex = -1;
        }

        text.setString(message.substr(lastRow, index - lastRow));
        double width = text.getGlobalBounds().width;
        double height = text.getGlobalBounds().height;

        if (message[index] == '\n') {

            if (wordIndex > 0) {
                lastRow = wordIndex + 1;
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
    text.setString("");

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

TextBox::TextBox(int width, int height, int characterSize, std::string fontPath) {
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

TextBox::~TextBox() {
}

const sf::Text& TextBox::GetText() const { return text; }

const sf::Font& TextBox::GetFont() const { return *font; }

void TextBox::SetTextFillColor(sf::Color color) {
    fillColor = color;
}

void TextBox::SetTextOutlineColor(sf::Color color) {
    outlineColor = color;
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
    if (lineIndex + numberOfFittingLines < lines.size())
        if (charIndex > lines[lineIndex + numberOfFittingLines])
            return true;

    return false;
}

const bool TextBox::HasLess() const {
    return lineIndex > 0;
}

void TextBox::ShowNextLine() {
    lineIndex++;

    if (lineIndex >= lines.size())
        lineIndex = (int)lines.size() - 1;
}

void TextBox::ShowPreviousLine() {
    lineIndex--;

    if (lineIndex < 0)
        lineIndex = 0;
}

void TextBox::CompleteCurrentBlock()
{
  int newCharIndex = message.size() - 1;
  int lastLine = lineIndex + GetNumberOfFittingLines();

  if (lastLine < this->lines.size()) {
    newCharIndex = this->lines[lastLine]-1;
  }

  int charactersSkipped = newCharIndex - charIndex;
  double elapsed = static_cast<double>(charactersSkipped) / this->charsPerSecond;
  this->progress += elapsed;
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
    FormatToFit();
}

void TextBox::Play(const bool play) {
    TextBox::play = play;
}

void TextBox::Stop() {
    Play(false);
}

const char TextBox::GetCurrentCharacter() const {
    return message[charIndex];
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
    // If we're paused don't update
    // If the message is empty don't update
    if (!play || message.empty()) return;

    // If we're at the end of the message, don't step  
    // through the words
    if (charIndex >= message.length()) {
        int begin = lines[lineIndex];
        int lastIndex = std::min((int)lines.size() - 1, lineIndex + numberOfFittingLines - 1);
        int last = lines[lastIndex];
        auto len = 0;

        if (lineIndex + (numberOfFittingLines) < lines.size()) {
            len = std::min(charIndex - begin, lines[lineIndex + (numberOfFittingLines)] - begin);
        }
        else {
            len = charIndex - begin;
        }

        text.setString(message.substr(begin, len));

        play = false;

        // We don't need to fill box
        return;
    }

    // Without this, the audio would play numerous times per frame and sounds bad
    bool playOnce = true;

    int charIndexIter = 0;
    progress += elapsed;

    // Work backwards, printing what we can show at this frame in respect to the CPS
    double simulate = progress;
    // Start at elapsed time `progress` and simulate until it his zero
    // That is our new state
    while (simulate > 0 && charsPerSecond > 0) {

        simulate -= 1.0 / charsPerSecond;

        // Skip over line breaks and empty spaces
        while (charIndexIter < message.size() && message[charIndexIter] == ' ') {
            charIndexIter++;
        }

        // Try the next character
        charIndexIter++;

        // If we're passed the current char index but there's more text to show...
        if (charIndexIter > charIndex && charIndex < message.size()) {

            // Update the current char index
            charIndex = charIndexIter;

            // We may overshoot, adjust
            if (charIndexIter >= message.size()) {
                charIndex--;
            }
            else {
                // Play a sound if we are able and the character is a letter
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

const bool TextBox::IsEndOfMessage() const {
    return (charIndex >= message.length());
}

const bool TextBox::IsEndOfBlock() const
{
  int testCharIndex = message.size() - 1;
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

    text.setPosition(getPosition());
    text.setScale(getScale());
    text.setRotation(getRotation());
    text.setFillColor(fillColor);
    text.setOutlineColor(outlineColor);

    target.draw(text);
}