#include "bnMessageQuestion.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"

Question::Question(std::string message, std::function<void()> onYes, std::function<void()> onNo) : 
  Message(message + "\n    YES        NO") {
  Question::onNo = onNo;
  Question::onYes = onYes;
  isQuestionReady = false;
  selectCursor.setTexture(LOAD_TEXTURE(TEXT_BOX_CURSOR));
  selectCursor.scale(2.0f, 2.0f);
  elapsed = 0;
  yes = canceled = false;
  ShowEndMessageCursor(false);
}

Question::~Question() {

}

const bool Question::SelectYes() { if (!isQuestionReady || yes) return false; else yes = true; return true; }
const bool Question::SelectNo() { if (!isQuestionReady || !yes) return false; else yes = false; return true; }

const bool Question::Cancel()
{
  bool result = SelectNo();
  canceled = true; // wait one more frame
  return result;
}

void Question::ConfirmSelection()
{
  if (!isQuestionReady) return;
  ExecuteSelection();
  GetTextBox()->DequeMessage();
}

void Question::ExecuteSelection() {
  if (yes) { 
      onYes(); 
  }
  else {
      onNo(); 
  }

  canceled = false; // reset
  isQuestionReady = false; // reset 
}

void Question::OnUpdate(double elapsed) {
  this->elapsed = elapsed;
  this->totalElapsed += this->elapsed;

  isQuestionReady = GetTextBox()->IsEndOfMessage();

  Message::OnUpdate(elapsed);

  if (canceled) {
      SelectNo();
      ExecuteSelection();
      return;
  }
}

void Question::OnDraw(sf::RenderTarget& target, sf::RenderStates states) {

  // We added "YES NO" to the last row of the message box
  // So it is visible when the message box is done printing.
  // Find out how many rows there are and place arrows to fit the text.
  int numOfFitLines = GetTextBox()->GetNumberOfFittingLines();
  int cursorY = static_cast<int>((3 - numOfFitLines) * -24.0f - 20.0f);
  unsigned bob = from_seconds(this->totalElapsed*0.25).count() % 5; // 5 pixel bobs
  float bobf = static_cast<float>(bob);

  float textBoxBottom = GetTextBox()->getPosition().y + GetTextBox()->GetFrameHeight() / 2.0f;

  if (yes) {
      selectCursor.setPosition(142.0f + bobf, textBoxBottom + cursorY);
  }
  else {
      selectCursor.setPosition(314.0f + bobf, textBoxBottom + cursorY);
  }

  if (isQuestionReady) {
      // Draw the Yes / No and a cursor
      target.draw(selectCursor,states);
  }

  Message::OnDraw(target, states);
}

void Question::SetTextBox(AnimatedTextBox * parent){
  Message::SetTextBox(parent);
}
