#include "bnMessageQuestion.h"
#include "bnInputManager.h"
#include "bnTextureResourceManager.h"

Question::Question(std::string message, std::function<void()> onYes, std::function<void()> onNo) : Message(message) {
  this->onNo = onNo;
  this->onYes = onYes;
  this->isQuestionReady = false;
  selectCursor.setTexture(LOAD_TEXTURE(TEXT_BOX_CURSOR));
  elapsed = 0;
  yes = canceled = false;
}

Question::~Question() {

}

const bool Question::SelectYes() { if (!isQuestionReady) return false; else yes = true; return true; }
const bool Question::SelectNo() { if (!isQuestionReady) return false; else yes = false; return true; }

void Question::ExecuteSelection() {
  if (yes) { 
    onYes(); 
    AUDIO.Play(AudioType::NEW_GAME);
  }
  else {
    { onNo(); }
  }

  canceled = false; // reset
  isQuestionReady = false; // reset 
}

void Question::OnUpdate(double elapsed) {
  this->elapsed = elapsed;;

  isQuestionReady = !GetTextBox()->IsPlaying() && GetTextBox()->IsEndOfMessage();

  if (canceled) {
    SelectNo();
    ExecuteSelection();
    return;
  }

  if (!isQuestionReady) return;

  if (INPUT.Has(EventTypes::RELEASED_CONFIRM)) {
    ExecuteSelection();
    GetTextBox()->DequeMessage();
  }
  else if (INPUT.Has(EventTypes::RELEASED_CANCEL)) {
    SelectNo();
    canceled = true; // wait one more frame
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
    selectCursor.setPosition(60.0f, 0.0f);
  }
  else {
    selectCursor.setPosition(150.0f, 0.0f);
  }

  if (isQuestionReady) {
    target.draw(options, states);
    // Draw the Yes / No and a cursor
    target.draw(selectCursor,states);
  }
  else {
    GetTextBox()->DrawMessage(target, states);
  }

}

void Question::SetTextBox(AnimatedTextBox * parent)
{

  Message::SetTextBox(parent);
  options = parent->MakeTextObject("YES          NO");
  options.setFillColor(sf::Color::Black);
  options.setOutlineColor(sf::Color::Black);
  options.setPosition(sf::Vector2f(70, 0));
  options.setScale(0.5f, 0.5f);
}
