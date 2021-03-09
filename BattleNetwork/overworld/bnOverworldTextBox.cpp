#include "bnOverworldTextBox.h"

#include "../bnMessage.h"
#include "../bnMessageQuestion.h"
#include "../bnMessageQuiz.h"

namespace Overworld {
  TextBox::TextBox(sf::Vector2f pos) : textbox(pos) {}

  void TextBox::SetNextSpeaker(sf::Sprite speaker, Animation animation) {
    nextSpeaker = speaker;
    nextAnimation = animation;
  }

  void TextBox::EnqueueMessage(const std::string& message, const std::function<void()>& onComplete) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    textbox.EnqueMessage(nextSpeaker, nextAnimation, new Message(message));

    handlerQueue.push([=](InputManager& input) {
      if (!input.Has(InputEvents::pressed_interact)) {
        return;
      }

      // continue the conversation if the text is complete
      if (textbox.IsEndOfMessage()) {
        textbox.DequeMessage();
        handlerQueue.pop();
        onComplete();
      }
      else if (textbox.IsEndOfBlock()) {
        textbox.ShowNextLines();
      }
      else {
        // double tapping talk will complete the block
        textbox.CompleteCurrentBlock();
      }
    });
  }

  void TextBox::EnqueueQuestion(const std::string& prompt, const std::function<void(bool)>& onResponse) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto question = new Question(prompt, [=]() {onResponse(1);}, [=]() {onResponse(0);});
    textbox.EnqueMessage(nextSpeaker, nextAnimation, question);


    handlerQueue.push([=](InputManager& input) {
      bool left = input.Has(InputEvents::pressed_ui_left);
      bool right = input.Has(InputEvents::pressed_ui_right);
      bool confirm = input.Has(InputEvents::pressed_confirm);
      bool cancel = input.Has(InputEvents::pressed_cancel);

      if (left && right) {}
      else if (left) {
        question->SelectYes();
      }
      else if (right) {
        question->SelectNo();
      }

      if (confirm && cancel) {}
      else if (confirm) {
        if (!textbox.IsEndOfMessage()) {
          textbox.CompleteCurrentBlock();
        }
        else {
          question->ConfirmSelection();
          handlerQueue.pop();
        }
      }
      else if (cancel) {
        question->SelectNo();
        question->ConfirmSelection();
        handlerQueue.pop();
      }
    });
  }

  void TextBox::EnqueueQuiz(
    const std::string& optionA,
    const std::string& optionB,
    const std::string& optionC,
    const std::function<void(int)>& onResponse
  ) {
    if (!textbox.HasMessage()) {
      textbox.Open();
    }

    auto quiz = new Quiz(optionA, optionB, optionC, onResponse);
    textbox.EnqueMessage(nextSpeaker, nextAnimation, quiz);

    handlerQueue.push([=](InputManager& input) {
      bool up = input.Has(InputEvents::pressed_ui_up);
      bool down = input.Has(InputEvents::pressed_ui_down);
      bool confirm = input.Has(InputEvents::pressed_confirm);

      if (up && down) {}
      else if (up) {
        quiz->SelectUp();
      }
      else if (down) {
        quiz->SelectDown();
      }

      if (confirm) {
        if (!textbox.IsEndOfMessage()) {
          textbox.CompleteCurrentBlock();
        }
        else {
          quiz->ConfirmSelection();
          handlerQueue.pop();
        }
      }
    });
  }

  bool TextBox::IsOpen() {
    return textbox.IsOpen();
  }

  bool TextBox::IsClosed() {
    return textbox.IsClosed();
  }

  void TextBox::Update(float elapsed) {
    textbox.Update(elapsed);
  }

  void TextBox::HandleInput(InputManager& input) {
    handlerQueue.front()(input);

    if (!textbox.HasMessage()) {
      // if there are no more messages, close
      textbox.Close();
    }
  }

  void TextBox::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    textbox.draw(target, states);
  }
}
