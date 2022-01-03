#include "bnInputTextBuffer.h"

static float GetCharacterWidth(Font& font, float letterSpacing, char c) {
  switch (c)
  {
  case L' ':
    return font.GetWhiteSpaceWidth() + letterSpacing;
  case L'\t':
    return (font.GetWhiteSpaceWidth() + letterSpacing) * 4;
  case L'\n':
    return 0;
  default:
    font.SetLetter(c);
    return font.GetLetterWidth() + letterSpacing;
  }
}

void InputTextBuffer::BeginCapture() {
  captureInput = true;
}

void InputTextBuffer::EndCapture() {
  captureInput = false;
}

void InputTextBuffer::Reset() {
  buffer.clear();
  lineIndexes = { 0 };
  caretPos = 0;
  font = Font(Font::Style::thin);
  maxLineWidth = FLT_MAX;
  characterLimit = SIZE_MAX;
  ignoreNewLine = false;
  modified = false;
  modifiedThisRun = false;
  carriageReturn = false;
  password = false;
}

bool InputTextBuffer::IsModified() {
  return modified;
}

void InputTextBuffer::SetText(const std::string& text)
{
  buffer = text.size() < characterLimit ? text : text.substr(0, characterLimit);
  caretPos = std::min(caretPos, buffer.size());
  modified = false;
}


void InputTextBuffer::InsertCharacter(char c) {
  bool isValidChar = c >= 32 && c < 127 || c == '\t' || c == '\n';

  if (buffer.size() < characterLimit && isValidChar) {
    buffer.insert(caretPos, 1, (char)c);
    caretPos += 1;
    MarkModified();
  }
}

const std::string& InputTextBuffer::ToString()
{
  modified = false;

  return buffer;
}

const size_t InputTextBuffer::GetCaretPosition() const {
  return caretPos;
}

void InputTextBuffer::SetCaretPosition(size_t pos) {
  caretPos = std::min(pos, buffer.size());
}

void InputTextBuffer::MoveCaretToCursor(const sf::Vector2f& pos) {
  if (pos.x < 0 || pos.y < 0) {
    return;
  }

  size_t row = size_t(pos.y / font.GetLineHeight());
  size_t col{};

  if (row >= lineIndexes.size()) {
    caretPos = buffer.size();
    return;
  }

  caretPos = lineIndexes[row];

  auto rowEnd = row + 1 < lineIndexes.size() ? lineIndexes[row + 1] : buffer.size();

  float width{};

  while (caretPos < rowEnd) {
    auto characterWidth = GetCharacterWidth(font, 1, password? '*' : buffer[caretPos]);

    if (width + characterWidth / 2.0f > pos.x) {
      break;
    }

    width += characterWidth;
    caretPos++;
  }
}

void InputTextBuffer::SetFont(const Font& font) {
  this->font = font;
}

void InputTextBuffer::SetLineWidth(float width) {
  maxLineWidth = width;
}

void InputTextBuffer::SetCharacterLimit(size_t limit) {
  characterLimit = limit;

  if (limit != SIZE_MAX) {
    buffer.reserve(limit);
  }
}

void InputTextBuffer::SetIgnoreNewLine(bool ignore) {
  ignoreNewLine = ignore;
}

void InputTextBuffer::ProtectPassword(bool isPassword)
{
  password = isPassword;
}

static bool isHoldingControl() {
  return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
}

void InputTextBuffer::HandleTextEntered(sf::Event e) {
  if (!captureInput) {
    return;
  }

  auto c = e.text.unicode;

  switch (c) {
  case 8: // backspace
    if (caretPos > 0) {
      caretPos -= 1;

      if (isHoldingControl()) {
        // delete word or space
        bool deletingWord = !isspace(buffer[caretPos]); // using !isspace, because isspace returns an int, check below

        auto caretStart = caretPos + 1;

        while (caretPos > 0 && !isspace(buffer[caretPos - 1]) == deletingWord) {
          caretPos -= 1;
        }

        auto len = caretStart - caretPos;

        if (len) {
          buffer.erase(caretPos, len);
          MarkModified();
        }
      }
      else {
        // delete char
        buffer.erase(caretPos, 1);
        MarkModified();
      }
    }
    break;
  case 127: // delete
    if (caretPos < buffer.size()) {
      if (isHoldingControl()) {
        // delete word or space
        bool deletingWord = !isspace(buffer[caretPos]); // using !isspace, because isspace returns an int, check below

        auto caretEnd = caretPos;

        caretEnd += 1;

        while (caretEnd < buffer.size() && !isspace(buffer[caretEnd]) == deletingWord) {
          caretEnd += 1;
        }

        auto len = caretEnd - caretPos;

        if (len) {
          buffer.erase(caretPos, len);
          MarkModified();
        }
      }
      else {
        // delete char
        buffer.erase(caretPos, 1);
        MarkModified();
      }
    }
    break;
  case '\r':
    if (!ignoreNewLine) {
      InsertCharacter('\n');
    }

    // track carriage return to prevent dup \n on windows
    // allows mac os to use \r
    carriageReturn = true;
    break;
  case '\n':
    if (!ignoreNewLine && !carriageReturn) {
      InsertCharacter('\n');
    }

    carriageReturn = false;
    break;
  default:
    InsertCharacter((char)c);
  }
}

void InputTextBuffer::HandleKeyPressed(sf::Event e) {
  if (!captureInput) {
    return;
  }

  switch (e.key.code) {
  case sf::Keyboard::Key::Left:
    if (caretPos > 0) {
      caretPos -= 1;
    }

    if (isHoldingControl() && caretPos > 0) {
      // jump word
      bool searchingForWord = isspace(buffer[caretPos - 1]);

      while (caretPos > 0) {
        if (isspace(buffer[caretPos - 1])) {
          if (!searchingForWord) {
            break;
          }
        }
        else {
          searchingForWord = false;
        }

        caretPos -= 1;
      }
    }
    break;
  case sf::Keyboard::Key::Right:
    if (isHoldingControl()) {
      // jump word
      bool searchingForWord = isspace(buffer[caretPos]);

      while (caretPos < buffer.size()) {
        caretPos += 1;

        if (isspace(buffer[caretPos])) {
          if (!searchingForWord) {
            break;
          }
        }
        else {
          searchingForWord = false;
        }
      }
    }
    else {
      if (caretPos < buffer.size()) {
        caretPos += 1;
      }
    }
    break;
  case sf::Keyboard::Key::Up:
    // move up a row
    LineUp();
    break;
  case sf::Keyboard::Key::Down:
    // move down a row
    LineDown();
    break;
  }
}

void InputTextBuffer::HandlePaste(const std::string& data) {
  for (char c : data) {
    InsertCharacter(c);
  }
}

void InputTextBuffer::HandleCompletedEventProcessing() {
  if (!modifiedThisRun) {
    return;
  }

  modifiedThisRun = false;
  modified = true;
  CalculateLineIndexes();
}

void InputTextBuffer::CalculateLineIndexes() {
  lineIndexes.clear();
  lineIndexes.push_back(0);

  size_t col{};
  size_t wordStart{};
  float lineWidth{};
  float wordWidth{};
  bool foundSpace{};

  // calculate line indexes
  for (auto i = 0; i < buffer.size(); i++) {
    char c = buffer[i];
    float characterWidth = GetCharacterWidth(font, 1, password? '*' : c);
    lineWidth += characterWidth;

    // detect words
    if (c == ' ' || c == '\n') {
      wordStart = 0;
      foundSpace = true;
    }
    else {
      // in a word
      if (wordStart == 0 && foundSpace) {
        // found the start of a word
        wordStart = col;
        wordWidth = 0;
      }

      // update word
      wordWidth += characterWidth;
    }

    col++;

    // detect wrapping
    if (lineWidth > maxLineWidth || c == '\n') {
      foundSpace = false;

      if (wordStart == 0) {
        // no word to break on
        lineWidth = c == ' ' ? 0 : characterWidth;
        col = 0;

        if (c == '\n' || c == ' ') {
          lineIndexes.push_back(i + 1);
        }
        else {
          lineIndexes.push_back(i);
        }
      }
      else {
        // carried from word to next line
        lineWidth = wordWidth;
        col = col - wordStart;
        wordStart = 0;
        wordWidth = 0;

        lineIndexes.push_back(i - col + 1);
      }
    }
  }
}

std::pair<size_t, size_t> InputTextBuffer::GetRowCol() {
  size_t row{};
  size_t col = caretPos;

  for (auto lineIndex : lineIndexes) {
    if (caretPos < lineIndex) {
      break;
    }

    row++;
  }

  row -= 1;
  col -= lineIndexes[row];

  return { row, col };
}

void InputTextBuffer::LineUp() {
  auto [row, initialCol] = GetRowCol();

  if (row == 0) {
    // first row, just jump to the start
    caretPos = 0;
    return;
  }

  auto previousRowCharIndex = lineIndexes[row - 1];
  auto previousRowLen = lineIndexes[row] - previousRowCharIndex;

  if (previousRowLen < initialCol) {
    caretPos = previousRowCharIndex + previousRowLen - 1;
    return;
  }

  caretPos = previousRowCharIndex + initialCol;
}

void InputTextBuffer::LineDown() {
  auto [row, initialCol] = GetRowCol();

  if (row == lineIndexes.size() - 1) {
    // last row, just jump to the end
    caretPos = buffer.size();
    return;
  }

  auto nextRowCharEndIndex = row + 2 < lineIndexes.size() ? lineIndexes[row + 2] : buffer.size();
  auto nextRowCharIndex = lineIndexes[row + 1];
  auto nextRowLen = nextRowCharEndIndex - nextRowCharIndex;

  if (nextRowLen < initialCol) {
    caretPos = nextRowCharIndex + nextRowLen;
    return;
  }

  caretPos = nextRowCharIndex + initialCol;
}

void InputTextBuffer::MarkModified() {
  modifiedThisRun = true;
}