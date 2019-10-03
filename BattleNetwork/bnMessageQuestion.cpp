#include "bnMessageQuestion.h"
#include "bnInputManager.h"
#include "bnTextureResourceManager.h"

Question::Question(std::string message, std::function<void()> onYes, std::function<void()> onNo) : Message(message) {
  this->onNo = onNo;
  this->onYes = onYes;
  this->isQuestionReady = false;
  selectCursor = sf::Sprite(LOAD_TEXTURE(TEXT_BOX_CURSOR));
  elapsed = 0;
  yes = false;
}

Question::~Question() {

}

const bool Question::SelectYes() { if (!isQuestionReady) return false; else yes = true; return true; }
const bool Question::SelectNo() { if (!isQuestionReady) return false; else yes = false; return true; }

void Question::ExecuteSelection() {
  if (yes) { onYes(); }
  else {
    { onNo(); }
  }
}

void Question::OnUpdate(double elapsed) {
  this->elapsed = elapsed;;

  isQuestionReady = !GetTextBox()->IsPlaying() && GetTextBox()->IsEndOfMessage();

  if (INPUT.Has(EventTypes::RELEASED_CONFIRM) && isQuestionReady) {
    ExecuteSelection();
    GetTextBox()->DequeMessage();
  }
  else if (INPUT.Has(EventTypes::RELEASED_CANCEL)) {
    AUDIO.Play(AudioType::CHIP_ERROR);
  }
  else if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
    SelectYes();
  }
  else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
    SelectNo();
  }
}

void Question::OnDraw(sf::RenderTarget& target, sf::RenderStates states) {

  if (yes) {
    auto x = swoosh::ease::interpolate((float)elapsed * 10.f, selectCursor.getPosition().x, 140.0f);
    selectCursor.setPosition(x, GetTextBox()->getPosition().y);
  }
  else {
    auto x = swoosh::ease::interpolate((float)elapsed * 10.f, selectCursor.getPosition().x, 265.0f);
    selectCursor.setPosition(x, GetTextBox()->getPosition().y);
  }

  if (isQuestionReady) {
    auto last = states;

    states.transform = options.getTransform();
    target.draw(options, states);

    states = last;
    states.transform = selectCursor.getTransform();
    // Draw the Yes / No and a cursor
    target.draw(selectCursor,states);
  }
  else {
    GetTextBox()->DrawMessage(target, states);
  }

}

void Question::SetTextBox(AnimatedTextBox * parent)
{
  MessageInterface::SetTextbox(parent);
  options = parent->MakeTextObject("YES NO");
  options.setFillColor(sf::Color::Black);
  options.setOutlineColor(sf::Color::Black);
  options.setPosition(parent->getPosition());
  options.setScale(parent->getScale());

  selectCursor.setScale(parent->getScale());
}
