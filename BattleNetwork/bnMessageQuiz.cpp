#include "bnMessageQuiz.h"

Quiz::Quiz(const std::string& optionA, const std::string& optionB, const std::string& optionC, const std::function<void(int)>& onResponse) :
  MessageInterface(("  " +optionA + "\n  " + optionB + "\n  " + optionC).c_str()),
  onResponse(onResponse) {
  selection = 0;
  options = {
    optionA,
    optionB,
    optionC
  };

  // select the first nonblank
  while (selection < 2) {
    if (options[selection] != "") {
      break;
    }

    selection += 1;
  }

  ResourceHandle handle;
  selectCursor.setTexture(handle.Textures().LoadFromFile(TexturePaths::TEXT_BOX_CURSOR));
}

/**
 * @brief Moves selection up if applicable
 * @return true if success, false otherwise
 */
const bool Quiz::SelectUp() {
  int oldSelection = selection;

  while (selection > 0) {
    selection -= 1;

    // skip blank
    if (options[selection] == "") continue;

    return true;
  }

  // could not select anything
  selection = oldSelection;
  return false;
}

/**
 * @brief Moves selection down if applicable
 * @return true if success, false otherwise
 */
const bool Quiz::SelectDown() {
  int oldSelection = selection;

  while (selection < 2) {
    selection += 1;

    // skip blank
    if (options[selection] == "") continue;

    return true;
  }

  // could not select anything
  selection = oldSelection;
  return false;
}

void Quiz::ConfirmSelection() {
  onResponse(selection);
  GetTextBox()->DequeMessage();
}

void Quiz::OnUpdate(double elapsed) {
  this->elapsed += elapsed;
}

void Quiz::OnDraw(sf::RenderTarget& target, sf::RenderStates states) {
  AnimatedTextBox* textbox = GetTextBox();

  textbox->DrawMessage(target, states);

  if (textbox->IsPlaying()) {
    return;
  }

  sf::Vector2f textboxPosition = textbox->GetTextPosition();

  unsigned bob = from_seconds(this->elapsed * 0.25).count() % 5; // 5 pixel bobs
  float bobf = static_cast<float>(bob);

  float cursorX = textboxPosition.x + bobf;
  float cursorY = textboxPosition.y + (float)selection * textbox->GetFont().GetLineHeight() + 1.f;
  selectCursor.setPosition(cursorX,  cursorY);

  target.draw(selectCursor, states);
}
