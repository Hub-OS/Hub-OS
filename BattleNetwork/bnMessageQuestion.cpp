#include "bnMessageQuestion.h"
#include "bnInputManager.h"
#include "bnTextureResourceManager.h"

Question::Question(std::string message, std::function<void()> onYes, std::function<void()> onNo) 
    : Message(message + "\n    YES        NO") {
  Question::onNo = onNo;
  Question::onYes = onYes;
  isQuestionReady = false;
  selectCursor.setTexture(LOAD_TEXTURE(TEXT_BOX_CURSOR));
  elapsed = 0;
  yes = canceled = false;
}

Question::~Question() {

}

const bool Question::SelectYes() { if (!isQuestionReady) return false; else yes = true; return true; }
const bool Question::SelectNo() { if (!isQuestionReady) return false; else yes = false; return true; }

void Question::Cancel()
{
    if (!isQuestionReady) return;
    SelectNo();
    canceled = true; // wait one more frame
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
        AUDIO.Play(AudioType::NEW_GAME);
    }
    else {
        onNo(); 
    }

    canceled = false; // reset
    isQuestionReady = false; // reset 
}

void Question::OnUpdate(double elapsed) {
    Question::elapsed = elapsed;;

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
    float cursorY = -20.0f;
    int numOfFitLines = GetTextBox()->GetNumberOfFittingLines();

    if (numOfFitLines == 2) {
        cursorY = -8.0f;
    }
    else if (numOfFitLines == 3) {
        cursorY = 6.0f;
    }

    if (yes) {
        selectCursor.setPosition(62.0f, cursorY);
    }
    else {
        selectCursor.setPosition(140.0f, cursorY);
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
